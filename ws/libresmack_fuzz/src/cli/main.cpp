#include "resmack/fuzz/config.hpp"
#include "resmack/fuzz/cli/parse.hpp"
#include "resmack/fuzz/cli/messages.hpp"
#include "resmack/fuzz/cmds.hpp"

namespace resmack {
namespace fuzz {
namespace cli {

  int Main(int argc, char** argv) {
    Config config {
      .action = ActionType::kFuzz,
      .fuzzConfig = {
        .nprocs = 1,
        .max_iters = 0,
        .max_crashes = 0,
        .stats_interval = 1000,
        .crash_output = (char*)"crashes",
        .create_threshhold = 10000,
        .run_direct = 0,
        .pin_cpus = true,
      },
      .targetConfig = {
        .mute_stdio = 0,
      },
      .corpusConfig = {
        .corpus_decay = 10000,
      },
      .displayConfig = {
        .print_interval = 0.0f,
        .show_stats = 0,
      },
      .grammarConfig = {
        .max_depth = 10,
      },
    };
    cli::ParseArgs(&config, argc, argv);

    switch (config.action) {
      case ActionType::kFuzz:
        cmds::Fuzz(&config.fuzzConfig);
        break;
      case ActionType::kDebugState:
        cmds::DebugState(&config.debugStateConfig);
        break;
      /*
      case cli::ActionType::kDumpCorpus:
        cmds::DumpCorpus(&config.dumpCorpusConfig);
        break;
      */
      case ActionType::kHelp:
        cmds::PrintHelp(argv[0], &config);
        break;
      default:
        Error("Unrecognized action");
        break;
    };

    return 0;
  }

}
}
}
