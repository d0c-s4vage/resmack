#include <stdio.h>
#include <iostream>

#include "resmack/logo.hpp"
#include "resmack/fuzz/cmds/help.hpp"

namespace resmack {
namespace fuzz {
namespace cmds {

  void PrintHelp(const char* prog_name, Config* config) {
    std::cout << resmack::GetResmackLogo() << "\n";

    std::cout << prog_name << " [-d MAX_DEPTH] [-n NPROCS] [-s] [-i INTERVAL] [--help]"  << "\n\n";
    std::cout << "             --help,-h             Show this help message" << "\n";
    std::cout << "           --direct                Run the fuzzing loop and target in the main thread (" << config->fuzzConfig.run_direct << ")" << "\n";
    std::cout << "           --nprocs,-n NPROCS      Number of times to fork (" << config->fuzzConfig.nprocs << ")" << "\n";
    std::cout << "          --crashes,-c CRASH_DIR   Where to store crashing inputs (" << config->fuzzConfig.crash_output << ")" << "\n";
    std::cout << "          --no-mute                Do not mute stdio of fuzz procs (" << !config->targetConfig.mute_stdio << ")" << "\n";
    std::cout << "        --max-depth,-d MAX_DEPTH   Maximum grammar depth during recursion (" << config->grammarConfig.max_depth << ")" << "\n";
    std::cout << "        --max-iters,-m MAX_ITERS   Maximum number of iterations (" << config->fuzzConfig.max_iters << ")" << "\n";
    std::cout << "       --no-pin-cpu                Don't pin processes to specific CPUs (" << !config->fuzzConfig.pin_cpus << ")" << "\n";
    std::cout << "      --max-crashes,-M MAX_CRASHES Maximum number of crashes (" << config->fuzzConfig.max_crashes << ")" << "\n";
    std::cout << "       --show-stats,-s             Show stat percentages (" << config->displayConfig.show_stats << ")" << "\n";
    std::cout << "       --state-path,-S             Path to where the state file should be stored (" << config->fuzzConfig.state_path << ")" << "\n";
    std::cout << "      --debug-state                Debug the specified state file" << "\n";
    std::cout << "      --dump-corpus,-D DIR         Dump the corpus to DIR" << "\n";
    std::cout << "     --corpus-decay                The number of mutation attempts that causes a" << "\n";
    std::cout << "                                   corpus entry to be fully deprioritized (" << config->corpusConfig.corpus_decay << ")" << "\n";
    std::cout << "     --corpus-strat,-C STRAT       Enable the corpus strategy STRAT, must be one of below options." << "\n";
    std::cout << "                                   May be used multiple times:" << "\n";
    std::cout << "                                      MOST_FEEDBACK            MOST_ANCESTORS   MOST_DESCENDANTS" << "\n";
    std::cout << "                                     LEAST_FEEDBACK           LEAST_ANCESTORS  LEAST_DESCENDANTS" << "\n";
    std::cout << "                                        MOST_RECENT   MOST_DIRECT_DESCENDANTS               RAND" << "\n";
    std::cout << "                                       LEAST_RECENT  LEAST_DIRECT_DESCENDANTS" << "\n";
    std::cout << "                                       LEAST_RECENT  LEAST_DIRECT_DESCENDANTS" << "\n";
    std::cout << "   --stats-interval,-i INTERVAL    Collect stats on every Nth iteration (" << config->fuzzConfig.stats_interval << ")" << "\n";
    std::cout << "   --print-interval,-p INTERVAL    Print at set intervals with no backoff (" << config->displayConfig.print_interval << ")" << "\n";
    std::cout << "--create-threshhold,-t THRESHOLD   The threshold to create new inputs (" << config->fuzzConfig.create_threshhold << ")" << "\n";
    std::cout << "\n";
    std::cout << "Example:" << std::endl << "\n";
    std::cout << prog_name << " -n 3 --show-stats" << "\n";
  }

}
}
}
