#include <iostream>
#include <chrono>
#include <cstddef>
#include <string>
#include <set>
#include <ctime>
#include <ratio>
#include "getopt.h"

#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/state.hpp"
#include "resmack/logo.hpp"
#include "resmack/build_context.hpp"
#include "resmack/rand.hpp"
#include "resmack/types.hpp"
#include "resmack/rules.hpp"
#include "resmack/items/str.hpp"
#include "resmack/items/or.hpp"
#include "resmack/items/opt.hpp"
#include "resmack/items/ref.hpp"
#include "resmack/items/and.hpp"

#include "resmack/fuzz/external.hpp"
#include "resmack/fuzz/mutate.hpp"
#include "resmack/fuzz/targets/direct.hpp"
#include "resmack/fuzz/feedbacks/coverage.hpp"
#include "resmack/fuzz/feedbacks/noop.hpp"
#include "resmack/fuzz/states/mmap.hpp"
#include "resmack/fuzz/corpus.hpp"
#include "resmack/fuzz/trace.hpp"

extern "C" int __lsan_is_turned_off() { return 1; }

void PrintHelp(char* prog_name) {
  std::cout << resmack::GetResmackLogo() << std::endl;

  std::cout << prog_name << std::endl << std::endl;
  std::cout << "  Fuzz the compiled target" << std::endl << std::endl;
  std::cout << prog_name << " [-d MAX_DEPTH] [-n NPROCS] [-s] [-i INTERVAL] [--help]"  << std::endl << std::endl;
  std::cout << "          --help, -h            Show this help message" << std::endl;
  std::cout << "        --nprocs, -n NPROCS     Number of times to fork" << std::endl;
  std::cout << "     --max-depth, -d MAX_DEPTH  Maximum grammar depth during recursion" << std::endl;
  std::cout << "    --show-stats, -s            Show stat percentages" << std::endl;
  std::cout << "--stats-interval, -i INTERVAL   Collect stats on every Nth iteration" << std::endl;
  std::cout << std::endl;
  std::cout << "Example:" << std::endl << std::endl;
  std::cout << prog_name << " -n 3 --show-stats" << std::endl;
}

struct FuzzOptions {
  int help;
  size_t nprocs;
  size_t max_depth;
  int show_stats;
  size_t stats_interval;
};

bool ParseOptions(int argc, char**argv, FuzzOptions* opts) {
    static struct option long_options[] = {
      { "help", no_argument, &opts->help, 'h' },
      { "show-stats", no_argument, &opts->show_stats, 's' },
      { "max-depth", optional_argument, 0, 'd' },
      { "nprocs", optional_argument, 0, 'n' },
      { "stats-interval", optional_argument, 0, 'i' },
      { 0, 0, 0, 0 },
    };
    int opt_index = 0;

    while (true) {
      int c = getopt_long(argc, argv, "hsd:n:i:", long_options, &opt_index);
      if (c == -1) {
        break;
      }

      switch (c) {
        case 0:
          break;
        case 'h':
          opts->help = true;
          break;
        case 's':
          opts->show_stats = true;
          break;
        case 'd':
          opts->max_depth = atoi(optarg);
          break;
        case 'n':
          opts->nprocs = atoi(optarg);
          break;
        case 'i':
          opts->stats_interval = atoi(optarg);
          break;
      }
    }

    return true;
}

void LoopPrintStatus(resmack::fuzz::states::MmapState* state, bool show_stats) {
  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
  std::chrono::high_resolution_clock::time_point end;
  uint64_t start_iters = state->GetNumIterations();
  int sleep_amt = 1;
  resmack::fuzz::Corpus* corpus = state->GetCorpus();
  resmack::fuzz::StateStats* stats = state->GetStats();

  while (true) {
    sleep(sleep_amt++);
    corpus->Sync();

    end = std::chrono::high_resolution_clock::now();
    uint64_t num_iters = state->GetNumIterations();
    uint64_t session_iters = num_iters - start_iters;
    std::chrono::duration<double> span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    printf(
      "Iters: %lu | %0.2f iters/s | Crashes: %lu | Corpus: %lu | %0.2f s\n",
      num_iters,
      (float)session_iters / span.count(),
      state->GetNumCrashes(),
      corpus->NumItems(),
      span.count()
    );

    if (!show_stats) {
      continue;
    }

    double tmp = 0;
    double total_time = 0;
#define STAT(NAME) total_time += stats->duration_##NAME;
#include "resmack/fuzz/stats.def"
#undef STAT

#define STAT(NAME) \
    tmp = stats->duration_##NAME / total_time; \
    printf("%5.2f%% - "#NAME"\n", tmp * 100);
#include "resmack/fuzz/stats.def"
#undef STAT
  }
}

void FuzzLoop(
  resmack::fuzz::Feedback* feedback,
  resmack::fuzz::State* state,
  resmack::fuzz::Corpus* corpus,
  FuzzOptions* opts,
  resmack::fuzz::Tracer* tracee,
) {
  resmack::Rand meta_rand;
  resmack::Rand build_rand(meta_rand.Next());
  build_rand.SetShouldRecord(true);

  std::string output;

  resmack::fuzz::DirectTarget target;
  resmack::fuzz::TargetSettings settings;
  resmack::fuzz::TargetStats stats(opts->stats_interval);
  resmack::BuildContext ctx(&output, &build_rand, opts->max_depth);

  resmack::Vector<resmack::RandSnapshot> mutated_replay;

  size_t counts = 0;
  while (true) {
    counts++;
    if ((counts % opts->stats_interval) == 0) {
      state.IncNumIterations(opts->stats_interval);
      state.SyncStats(&stats);
      stats.Clear();
    }
    stats.Tick();

    size_t last_corpus_idx = 0;
    bool used_corpus = false;
    if (corpus->NumItems() > 0 && meta_rand.Maybe()) {
      used_corpus = true;
      resmack::Vector<resmack::RandSnapshot>* replay;
      RECORD_STAT(&stats, resmack::fuzz::SampleTypes::CORPUS, {
        replay = corpus->GetItem(&meta_rand, &last_corpus_idx);
      });
      RECORD_STAT(&stats, resmack::fuzz::SampleTypes::MUTATE, {
        mutator->Mutate(&meta_rand, replay, &mutated_replay);
      });
      ctx.SetReplay(&mutated_replay);
    } else {
      ctx.SetReplay(NULL);
    }

    // this occurs *in* the traced process (after forking/*).
    // If an exception occurs, these values are extracted and
    // used to save crash information and update corpus stats
    tracee->SaveLastCorpusIndex(used_corpus, last_corpus_idx);
    tracee->SaveLastReplay(&mutated_replay);

    output.clear();
    build_rand.SnapshotClear();

    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::GENERATE, {
      rules.Build(rule_idx, &ctx);
    });
    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::TARGET, {
      target.Launch(feedback, &output, &settings, &stats);
    });
    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::TARGET_RESET, {
      target.Reset();
    });
    size_t cov_key = feedback.GetStats().key;

    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::CORPUS, {
      if (corpus->AddRandSnapshotIfNotSeen(build_rand.GetSnapshots(), cov_key)) {
        std::cout << "New coverage with: " << output << ", key: " << cov_key << ", num: " << cov.GetStats().num << ", iters: " << counts << std::endl;
      }
    });

    if (stats->crashed) {
      state->IncNumCrashes();
      std::cout << "CRASH! with " << output << " and " << state->GetNumIterations() << " iters" << std::endl;
    }
  }
}

static bool SHOULD_RUN = true;
void HandleTracees(resmack::Vector<resmack::fuzz::Trace*>* tracees) {
  for (resmack::fuzz::Trace* tracee : *tracees) {
    tracee->Run([tracee]() {
      if (SHOULD_RUN) {
        tracee->Restart();
      }
    });
  }
}

__attribute__((visibility("default"))) int main(int argc, char** argv) {
  FuzzOptions opts {
    .help = false,
    .nprocs = 1,
    .max_depth = 10,
    .show_stats = false,
    .stats_interval = 0x1000,
  };
  ParseOptions(argc, argv, &opts);
  if (opts.help) {
    PrintHelp(argv[0]);
    return 1;
  }

  resmack::fuzz::ExternalFunctions EF;

  resmack::Rules rules = new resmack::Rules();
  size_t rule_idx = EF.ResmackGrammarInit(&rules);

  resmack::fuzz::Coverage cov;
  resmack::fuzz::NoopCoverage noop_cov;
  resmack::fuzz::states::MmapState mmap_state("/tmp/resmack.state");
  resmack::fuzz::mutators::BasicRandMutator rand_mutator;

  resmack::fuzz::Feedback* feedback = &cov;
  resmack::fuzz::State* state = &state;
  resmack::fuzz::Corpus* corpus = state.GetCorpus();
  resmack::fuzz::Mutator* mutator = &rand_mutator;

  bool is_main_proc = true;

  resmack::fuzz::trace_targets::ForkTarget trace_target(
    [feedback, state, corpus, mutator](resmack::fuzz::Trace* tracee) {
      FuzzLoop(feedback, state, corpus, mutator, tracee);
    }
  );

  resmack::Vector<resmack::fuzz::Trace*> tracees;

  int child_num;
  std::cout << "Creating " << opts.nprocs << " proceses for fuzzing" << std::endl;
  for (child_num = 0; child_num < opts.nprocs; child_num++ ) {
    resmack::fuzz::Trace* tracee = new resmack::fuzz::Trace(&trace_target);
    tracees.emplace(tracee);
  }

  if (is_main_proc) {
    // TODO do this in a separate thread
    LoopPrintStatus(&state, opts.show_stats);
  }

  HandleTracees(&tracees);
}
