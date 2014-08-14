#include "metrics.h"

#include <boost/foreach.hpp>

using namespace boost;
using namespace std;

namespace diagnostic {
namespace benchmark {
string t_Cd::__KEY__("Cd");
string t_wasted_effort::__KEY__("wasted_effort");
string t_entropy::__KEY__("entropy");
string t_quality::__KEY__("_quality");
string t_quality_fair::__KEY__("_quality_fair");

const string & t_Cd::key () const {
    return __KEY__;
}

const string & t_wasted_effort::key () const {
    return __KEY__;
}

const string & t_entropy::key () const {
    return __KEY__;
}

const string & t_quality::key () const {
    return result_metric;
}

const string & t_quality_fair::key () const {
    return result_metric;
}

string t_Cd::operator () (const structs::t_spectra & spectra,
                          const structs::t_candidate & correct,
                          const t_candidate_generator::t_ret_type & D,
                          const t_candidate_ranker::t_ret_type & probs,
                          const structs::t_diagnostic_report & dr,
                          const t_metric::t_arguments & arguments) const {
    t_candidate_ranker::t_ret_type::value_type current = NAN;
    t_count total_elements = 0, elements = 0;
    bool quit = false;


    for (t_id i = 1; i <= dr.size(); i++) {
        if (dr.get_probability(i) == current)
            elements++;
        else {
            if (quit)
                break;

            current = dr.get_probability(i);
            elements = 1;
        }

        total_elements++;

        if (correct == dr.get_candidate(i))
            quit = true;
    }

    if (!quit) // Candidate not even considered
        return "NaN";

    assert(((total_elements - 1) - ((elements - 1) / 2.0)) >= 0);
    return lexical_cast<string> ((total_elements - 1) - ((elements - 1) / 2.0));
}

string t_wasted_effort::operator () (const structs::t_spectra & spectra,
                                     const structs::t_candidate & correct,
                                     const t_candidate_generator::t_ret_type & D,
                                     const t_candidate_ranker::t_ret_type & probs,
                                     const structs::t_diagnostic_report & dr,
                                     const t_metric::t_arguments & arguments) const {
    t_count elements = 0;
    t_candidate_ranker::t_ret_type::value_type current = NAN;
    structs::t_candidate remaining(correct), healthy_components;


    t_id i = 0;


    while ((i = dr.next(i, healthy_components))) {
        if (!(dr.get_probability(i) == current)) {
            if (remaining.size() == 0)
                break;

            current = dr.get_probability(i);
            elements = 0;
        }

        const structs::t_candidate & candidate = dr.get_candidate(i);

        BOOST_FOREACH(structs::t_candidate::value_type c,
                      candidate) {
            if (remaining.erase(c)) ;
            else if (correct.count(c)) ;
            else if (healthy_components.insert(c).second) elements++;
        }
    }

    if (remaining.size())
        // Candidate not even considered
        return "NaN";

    return lexical_cast<string> (healthy_components.size() - elements / 2.0);
}

string t_entropy::operator () (const structs::t_spectra & spectra,
                               const structs::t_candidate & correct,
                               const t_candidate_generator::t_ret_type & D,
                               const t_candidate_ranker::t_ret_type & probs,
                               const structs::t_diagnostic_report & dr,
                               const t_metric::t_arguments & arguments) const {
    return lexical_cast<string> (dr.get_entropy());
}

t_quality::t_quality (string target_metric) : target_metric(target_metric), result_metric(target_metric + __KEY__) {}

string t_quality::operator () (const structs::t_spectra & spectra,
                               const structs::t_candidate & correct,
                               const t_candidate_generator::t_ret_type & D,
                               const t_candidate_ranker::t_ret_type & probs,
                               const structs::t_diagnostic_report & dr,
                               const t_metric::t_arguments & arguments) const {
    double metric_value = get_argument<double> (target_metric, arguments);
    t_count remaining_components = (spectra.get_component_count() - correct.size());


    if (remaining_components)
        return lexical_cast<string> (1 - (metric_value / remaining_components));

    return lexical_cast<string> (1);
}

t_quality_fair::t_quality_fair (string target_metric) : target_metric(target_metric), result_metric(target_metric + __KEY__) {}

string t_quality_fair::operator () (const structs::t_spectra & spectra,
                                    const structs::t_candidate & correct,
                                    const t_candidate_generator::t_ret_type & D,
                                    const t_candidate_ranker::t_ret_type & probs,
                                    const structs::t_diagnostic_report & dr,
                                    const t_metric::t_arguments & arguments) const {
    double metric_value = get_argument<double> (target_metric, arguments);
    t_count remaining_components = (spectra.get_suspicious_components_count() - correct.size());


    if (remaining_components)
        return lexical_cast<string> (1 - (metric_value / remaining_components));

    return lexical_cast<string> (1);
}
}
}