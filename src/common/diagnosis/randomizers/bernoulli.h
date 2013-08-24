#ifndef __BERNOULLI_H_a6d1be206c8818d13562fb163cc8b337ff1a0f31__
#define __BERNOULLI_H_a6d1be206c8818d13562fb163cc8b337ff1a0f31__

#include "randomizer.h"
#include "diagnosis/structs/count_spectra.h"

namespace diagnosis {
namespace randomizers {
class t_bernoulli : public t_spectra_randomizer {
public:
    t_bernoulli (float activation_rate,
                 float error_rate,
                 t_count n_tran,
                 t_count n_comp);

    virtual structs::t_spectra * operator () (boost::random::mt19937 & gen,
                                              structs::t_candidate & correct_candidate) const;

    virtual std::ostream & write (std::ostream & out) const;

public:
    t_count n_comp;
    t_count n_tran;
    float error_rate;
    float activation_rate;
};
}
}

#endif