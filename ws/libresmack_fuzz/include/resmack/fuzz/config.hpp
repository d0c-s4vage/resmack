#ifndef RESMACK_FUZZ_CONFIG_H
#define RESMACK_FUZZ_CONFIG_H

#include <stddef.h>
#include <inttypes.h>

namespace resmack {
namespace fuzz {

  struct GrammarConfig {
    size_t max_depth;
  };

  struct DisplayConfig {
    float print_interval;
    int show_stats;
  };

  struct CorpusConfig {
    uint32_t corpus_strats;
    size_t corpus_decay;
  };

  struct TargetConfig {
    int mute_stdio;
  };

  struct DebugStateConfig {
    const char* state_path;
  };

  struct DumpCorpusConfig {
    const char* dump_corpus_path;
    const char* state_path;
  };

  struct FuzzConfig {
    size_t nprocs;
    size_t max_iters;
    size_t max_crashes;
    size_t stats_interval;
    char* crash_output;
    size_t create_threshhold;
    int run_direct;
    const char* state_path;
    bool pin_cpus;
  };

  enum class ActionType : int {
    kHelp,
    kFuzz,
    kDumpCorpus,
    kDebugState
  };

  struct Config {
    ActionType action;

    FuzzConfig fuzzConfig;
    DumpCorpusConfig dumpCorpusConfig;
    DebugStateConfig debugStateConfig;

    TargetConfig targetConfig;
    CorpusConfig corpusConfig;
    DisplayConfig displayConfig;
    GrammarConfig grammarConfig;
  };

}
}

#endif
