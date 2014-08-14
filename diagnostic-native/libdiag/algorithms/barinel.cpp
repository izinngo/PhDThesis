#include "barinel.h"
#include "../utils/iostream.h"

#include <boost/foreach.hpp>

namespace diagnostic {
namespace algorithms {
using namespace structs;

t_barinel_model::t_barinel_model (const t_spectra & spectra,
                                  const t_candidate & candidate,
                                  bool use_fuzzy_error,
                                  bool use_confidence,
                                  const t_spectra_filter * filter) {
    assert(candidate.size() < sizeof(t_component_id) * 8);

    t_spectra_filter tmp_filter;

    if (filter)
        tmp_filter = *filter;

    // Init filter
    tmp_filter.components.resize(spectra.get_component_count());
    tmp_filter.components.filter_all_but(candidate);

    t_spectra_iterator it(spectra.get_component_count(),
                          spectra.get_transaction_count(),
                          &tmp_filter);

    // Set container size
    pass.resize(1 << candidate.size(), 0);
    fail.resize(1 << candidate.size(), 0);

    while (it.transaction.next()) {
        t_id symbol = 0;

        for (t_id comp = 0; it.component.next(); comp++) {
            if (spectra.get_activations(it.component.get(),
                                        it.transaction.get()))
                symbol += 1 << comp;
        }

        if (symbol) {
            t_confidence conf = use_confidence ? spectra.get_confidence(it.transaction.get()) : 1;
            t_error err = use_fuzzy_error ? spectra.get_error(it.transaction.get()) : spectra.is_error(it.transaction.get());
            pass[symbol] += conf * (1 - err);
            fail[symbol] += conf * err;
        }
    }
}

void t_barinel_model::gradient (const t_barinel_goodnesses & goodnesses,
                                t_barinel_goodnesses & ret) const {
    t_goodness_mp tmp_f(0);


    ret = t_barinel_goodnesses(goodnesses.size(), t_goodness_mp(0));

    for (t_id pattern = 1; pattern < (1u << (goodnesses.size())); pattern++) {
        // PASS stuff
        for (t_id c = 0; c < goodnesses.size(); c++)
            ret[c] += pass[pattern] / goodnesses[c];

        // FAIL stuff
        tmp_f = 1;

        t_id pattern_tmp = pattern;

        for (t_id c = 0; pattern_tmp; c++, pattern_tmp >>= 1)
            if (pattern_tmp & 1)
                tmp_f *= goodnesses[c];

        pattern_tmp = pattern;

        for (t_id c = 0; pattern_tmp; c++, pattern_tmp >>= 1)
            if (pattern_tmp & 1)
                ret[c] += -fail[pattern] * (tmp_f / goodnesses[c]) / (1 - tmp_f);
    }
}

void t_barinel_model::update (const t_barinel_goodnesses & g,
                              const t_barinel_goodnesses & grad,
                              t_barinel_goodnesses & ret,
                              double lambda) const {
    t_goodness_mp lambda_mp(lambda);


    for (t_id c = 0; c < g.size(); c++) {
        if (grad[c] == 0)
            continue;

        ret[c] = g[c] + (lambda_mp * g[c] / grad[c]);

        if (ret[c] <= 0)
            ret[c] = 0;
        else if (ret[c] >= 1)
            ret[c] = 1;
    }
}

t_barinel::t_barinel (size_t precision) {
    g_j = 0.001;
    epsilon = 0.0001;
    lambda = 1;
    iterations = 0;
    use_fuzzy_error = true;
    use_confidence = true;
}

void t_barinel::operator () (const structs::t_spectra & spectra,
                             const structs::t_trie & D,
                             t_ret_type & probs,
                             const structs::t_spectra_filter * filter) const {
    BOOST_FOREACH(auto & candidate, D) {
        t_probability_mp ret;


        (* this)(spectra, candidate, ret);
        probs.push_back(ret);
    }
}

void t_barinel::operator () (const t_spectra & spectra,
                             const t_candidate & candidate,
                             t_probability_mp & ret,
                             const t_spectra_filter * filter) const {
    t_barinel_model m(spectra,
                      candidate,
                      use_fuzzy_error,
                      use_confidence,
                      filter);


    t_barinel_goodnesses g(candidate.size(), t_goodness_mp(0.5));
    t_barinel_goodnesses g_old(candidate.size(), t_goodness_mp(0.5));

    t_probability_mp pr(0);
    t_probability_mp pr_old(0);

    t_barinel_goodnesses grad(candidate.size(), t_goodness_mp(0.5));

    t_count it = 0;


    // Calculate probability
    probability(spectra, candidate, g, pr, filter);

    while (it++ < iterations || !iterations) {
        // Update olds
        pr_old = pr;
        g_old = g;

        // Update goodness values;
        m.gradient(g_old, grad);

        m.update(g_old, grad, g, lambda);

        // Calculate probability
        probability(spectra, candidate, g, pr, filter);

        // Check stop condition
        if (2 * (pr - pr_old) / abs(pr + pr_old) < epsilon)
            break;
    }

    t_probability_mp prior_pr;
    prior(candidate, prior_pr);

    ret = prior_pr * pr;
}

void t_barinel::probability (const structs::t_spectra & spectra,
                             const structs::t_candidate & candidate,
                             const t_barinel_goodnesses & goodnesses,
                             t_probability_mp & ret,
                             const t_spectra_filter * filter,
                             bool use_confidence,
                             bool use_fuzzy_error,
                             bool use_count) const {
    assert(candidate.size() > 0);

    t_goodness_mp tmp(goodnesses[0]);

    ret = 1;

    t_spectra_iterator it(spectra.get_component_count(),
                          spectra.get_transaction_count(),
                          filter);

    while (it.transaction.next()) {
        tmp = 1;
        structs::t_candidate::const_iterator c_it = candidate.begin();
        t_id comp = 0;

        while (c_it != candidate.end()) {
            t_count count = spectra.get_activations(*c_it, it.transaction.get());

            assert(goodnesses[comp] >= 0);
            assert(goodnesses[comp] <= 1);

            if (count) {
                if (use_count)
                    tmp *= pow(goodnesses[comp], count);
                else
                    tmp *= goodnesses[comp];
            }

            c_it++;
            comp++;
        }

        // Fuzzy health
        t_error e = use_fuzzy_error ? spectra.get_error(it.transaction.get()) : spectra.is_error(it.transaction.get());


        tmp = e * (1 - tmp) + (1 - e) * tmp;

        // Confidence scaling
        if (use_confidence) {
            t_confidence c = spectra.get_confidence(it.transaction.get());
            tmp = (1 - c) + (c * tmp);
        }

        ret *= tmp;
    }

    assert(ret >= 0);
    assert(ret <= 1);
}

void t_barinel::prior (const t_candidate & candidate,
                       t_goodness_mp & ret) const {
    ret = pow(g_j, candidate.size());
}
}
}