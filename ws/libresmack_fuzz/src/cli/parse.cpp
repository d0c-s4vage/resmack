#include <getopt.h>
#include <cstring>
#include <string>
#include <iostream>

#include "resmack/logo.hpp"
#include "resmack/fuzz/cli/parse.hpp"
#include "resmack/fuzz/corpus.hpp"

namespace resmack {
namespace fuzz {
namespace cli {

  void ParseArgs(Config* config, int argc, char** argv) {
#define OPT_NO_MUTE      1000
#define OPT_RUN_DIRECT   1001
#define OPT_NO_PIN_CPU   1002
#define OPT_CORPUS_DECAY 1003
#define OPT_DEBUG_STATE  1004
    static struct option long_options[] = {
      { "help", no_argument, 0, 'h' },
      { "nprocs", required_argument, 0, 'n' },
      { "crashes", required_argument, 0, 'c' },
      { "direct", no_argument, 0, OPT_RUN_DIRECT },
      { "no-mute", no_argument, 0, OPT_NO_MUTE },
      { "max-depth", required_argument, 0, 'd' },
      { "max-iters", required_argument, 0, 'm' },
      { "max-crashes", required_argument, 0, 'M' },
      { "show-stats", no_argument, &config->display_config.show_stats, 's' },
      { "state-path", required_argument, 0, 'S' },
      { "corpus-strat", required_argument, 0, 'C' },
      { "debug-state", no_argument, 0, OPT_DEBUG_STATE },
      { "dump-corpus", required_argument, 0, 'D' },
      { "stats-interval", required_argument, 0, 'i' },
      { "print-interval", required_argument, 0, 'p' },
      { "create-threshhold", required_argument, 0, 't' },
      { "no-pin-cpu", no_argument, 0, OPT_NO_PIN_CPU },
      { "corpus-decay", required_argument, 0, OPT_CORPUS_DECAY },
      { 0, 0, 0, 0 },
    };
    int opt_index = 0;

    static char state_path[4096];
    snprintf(state_path, sizeof(state_path), "%s.resmack-state", argv[0]);
    config->fuzz_config.state_path = state_path;

    while (true) {
      int c = getopt_long(argc, argv, "hsd:n:i:m:c:t:D:S:p:M:C:", long_options, &opt_index);
      if (c == -1) {
        break;
      }

      switch (c) {
        case 0:
          break;
        case 'h':
          config->action = ActionType::kHelp;
          break;
        case 's':
          config->display_config.show_stats = true;
          break;
        case 'd':
          config->grammar_config.max_depth = strtoull(optarg, NULL, 10);
          break;
        case 'm':
          config->fuzz_config.max_iters = strtoull(optarg, NULL, 10);
          break;
        case 'M':
          config->fuzz_config.max_crashes = strtoull(optarg, NULL, 10);
          break;
        case 'n':
          config->fuzz_config.nprocs = strtoull(optarg, NULL, 10);
          break;
        case 'i':
          config->fuzz_config.stats_interval = strtoull(optarg, NULL, 10);
          break;
        case 'c':
          config->fuzz_config.crash_output = optarg;
          break;
        case 't':
          config->fuzz_config.create_threshhold = strtoull(optarg, NULL, 10);
          break;
        case 'D':
          config->action = ActionType::kDumpCorpus;
          config->dump_corpus_config.dump_corpus_path = optarg;
          break;
        case OPT_DEBUG_STATE:
          config->action = ActionType::kDebugState;
        case OPT_NO_MUTE:
          config->target_config.mute_stdio = false;
          break;
        case OPT_RUN_DIRECT:
          config->fuzz_config.run_direct = true;
          break;
        case 'S':
          config->fuzz_config.state_path = optarg;
          config->debug_state_config.state_path = optarg;
          config->dump_corpus_config.state_path = optarg;
          break;
        case 'C':
          if (strcmp(optarg, "MOST_FEEDBACK") == 0) {
            config->corpus_config.corpus_strats |= static_cast<uint32_t>(CorpusStrat::kMostFeedback);
          } else if (strcmp(optarg, "LEAST_FEEDBACK") == 0) {
            config->corpus_config.corpus_strats |= static_cast<uint32_t>(CorpusStrat::kLeastFeedback);
          } else if (strcmp(optarg, "MOST_RECENT") == 0) {
            config->corpus_config.corpus_strats |= static_cast<uint32_t>(CorpusStrat::kMostRecent);
          } else if (strcmp(optarg, "LEAST_RECENT") == 0) {
            config->corpus_config.corpus_strats |= static_cast<uint32_t>(CorpusStrat::kLeastRecent);
          } else if (strcmp(optarg, "MOST_ANCESTORS") == 0) {
            config->corpus_config.corpus_strats |= static_cast<uint32_t>(CorpusStrat::kMostAncestors);
          } else if (strcmp(optarg, "LEAST_ANCESTORS") == 0) {
            config->corpus_config.corpus_strats |= static_cast<uint32_t>(CorpusStrat::kLeastAncestors);
          } else if (strcmp(optarg, "MOST_DIRECT_DESCENDANTS") == 0) {
            config->corpus_config.corpus_strats |= static_cast<uint32_t>(CorpusStrat::kMostDirectDescendants);
          } else if (strcmp(optarg, "LEAST_DIRECT_DESCENDANTS") == 0) {
            config->corpus_config.corpus_strats |= static_cast<uint32_t>(CorpusStrat::kLeastDirectDescendants);
          } else if (strcmp(optarg, "MOST_DESCENDANTS") == 0) {
            config->corpus_config.corpus_strats |= static_cast<uint32_t>(CorpusStrat::kMostDescendants);
          } else if (strcmp(optarg, "LEAST_DESCENDANTS") == 0) {
            config->corpus_config.corpus_strats |= static_cast<uint32_t>(CorpusStrat::kLeastDescendants);
          } else if (strcmp(optarg, "RAND") == 0) {
            config->corpus_config.corpus_strats |= static_cast<uint32_t>(CorpusStrat::kRand);
          } else {
            std::cout << "Invalid corpus strategy: " << optarg << "\n";
            std::exit(1);
          }
          break;
        case 'p':
          config->display_config.print_interval = std::stof(optarg);
          break;
        case OPT_NO_PIN_CPU:
          config->fuzz_config.pin_cpus = false;
          break;
        case OPT_CORPUS_DECAY:
          config->corpus_config.corpus_decay = strtoull(optarg, NULL, 10);
          break;
      }
    }
  }

}
}
}
