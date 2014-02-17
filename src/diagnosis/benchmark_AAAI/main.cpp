#include "topologies.h"

#include "types.h"
#include "utils/iostream.h"
#include "diagnosis/benchmark/path_generator.h"
#include "diagnosis/benchmark/generators/generator.h"
#include "diagnosis/benchmark/hooks/job_tracker.h"
#include "diagnosis/benchmark/hooks/flusher.h"
#include "diagnosis/benchmark/hooks/hook_combiner.h"
#include "diagnosis/benchmark/hooks/metrics_hook.h"
#include "diagnosis/benchmark/hooks/verbose_hook.h"
#include "diagnosis/benchmark/hooks/statistics_hook.h"
#include "diagnosis/benchmark/hooks/save_hook.h"
#include "diagnosis/benchmark/hooks/statistics_hook.h"
#include "diagnosis/benchmark/metrics.h"
#include "diagnosis/benchmark/benchmark.h"
#include "diagnosis/benchmark/benchmark_settings.h"
#include "diagnosis/algorithms/single_fault.h"
#include "diagnosis/algorithms/mhs.h"
#include "diagnosis/algorithms/barinel.h"
#include "diagnosis/heuristics/sort.h"
#include "diagnosis/heuristics/similarity.h"
#include "diagnosis/structs/count_spectra.h"
#include "diagnosis/structs/trie.h"
#include <list>

using namespace std;
using namespace diagnosis;
using namespace diagnosis::algorithms;
using namespace diagnosis::heuristics;
using namespace diagnosis::benchmark;
using namespace diagnosis::structs;

int main (int argc, char ** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <folder> <until_nerrors> <n_faults>" << std::endl;
        return 1;
    }

    std::string dest(argv[1]);
    int n_errors = atoi(argv[2]);
    int n_faults = atoi(argv[3]);

    time_t seed = time(NULL);
    std::mt19937 gen(seed);


    // Generation settings
    t_ptr<t_topology_based_generator> gen_settings(new t_topology_based_generator());
    gen_settings->set_until_nerrors(n_errors);
    gen_settings->set_max_activations(20);

    // Topology Randomizer
    t_n_tier_setup n_tier(gen_settings);
    n_tier.set_fault_type(t_fault(0, 0.9, 0.1, 0));
    n_tier.set_levels(3, 10);
    n_tier.set_per_level(3, 10);
    n_tier.set_n_faults(n_faults, n_faults);

    // Create spectra generators

    t_ptr<t_spectra_generator> spectra_generator;
    spectra_generator = generate_generators(5, 4, n_tier, gen);

    // Candidate Generators
    heuristics::t_heuristic heuristic;
    heuristic.push(new heuristics::t_ochiai());
    heuristic.push(new heuristics::t_sort());

    t_mhs * mhs_ptr = new t_mhs(heuristic);
    t_const_ptr<t_candidate_generator> mhs(mhs_ptr);
    t_const_ptr<t_candidate_generator> single_fault(new t_single_fault());

    mhs_ptr->max_time = 1e6;


    // Candidate Rankers
    t_barinel * barinel_ptr = new t_barinel();
    t_const_ptr<t_candidate_ranker> barinel(barinel_ptr);
    t_const_ptr<t_candidate_ranker> fuzzinel(new t_barinel());
    t_const_ptr<t_candidate_ranker> ochiai(new t_ochiai());


    barinel_ptr->use_confidence = false;
    barinel_ptr->use_fuzzy_error = false;

    // Metrics
    t_metrics_hook * metrics_hook = new t_metrics_hook();
    (*metrics_hook) << new t_Cd();
    (*metrics_hook) << new t_wasted_effort();
    (*metrics_hook) << new t_entropy();
    (*metrics_hook) << new t_quality(t_wasted_effort().key());
    (*metrics_hook) << new t_quality_fair(t_wasted_effort().key());

    // Benchmark Hooks
    t_hook_combiner * hook_ptr = new t_hook_combiner();
    t_const_ptr<t_benchmark_hook> hook(hook_ptr);

    (*hook_ptr) << new t_job_tracker_hook();
    (*hook_ptr) << new t_verbose_hook();
    // hook_ptr << new t_save_hook(dest);
    (*hook_ptr) << new t_statistics_hook();
    (*hook_ptr) << metrics_hook;
    (*hook_ptr) << new t_flusher_hook();


    // Collector
    t_const_ptr<t_path_generator> path_generator(new t_path_single_dir(dest));
    t_ptr<t_collector> collector(new t_collector(path_generator));

    // Job Queue
    t_ptr<t_job_queue> job_queue(new t_job_queue());

    // Benchmark
    t_benchmark_settings settings(collector, hook, job_queue);

    settings.add_generator(mhs, "mhs");
    // settings.add_generator(single_fault, "single_fault");

    settings.add_ranker(barinel, "barinel");
    settings.add_ranker(fuzzinel, "fuzzinel");
    // settings.add_ranker(ochiai, "ochiai");

    settings.add_connection("mhs", "barinel");
    settings.add_connection("mhs", "fuzzinel");
    // benchmark.add_connection("single_fault", "ochiai");


    // Execution Controller
    t_execution_controller controller(3);

    // Launch
    run_benchmark(*spectra_generator,
                  gen,
                  settings,
                  controller);


    return 0;
}