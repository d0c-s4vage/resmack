#include <chrono>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <ctype.h>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <openssl/sha.h>
#include <pthread.h>
#include <ratio>
#include <set>
#include <signal.h>
#include <string>

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

#include "resmack/fuzz/corpus.hpp"
#include "resmack/fuzz/external.hpp"
#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/feedbacks/coverage.hpp"
#include "resmack/fuzz/feedbacks/noop.hpp"
#include "resmack/fuzz/mutate.hpp"
#include "resmack/fuzz/state.hpp"
#include "resmack/fuzz/states/mmap.hpp"
#include "resmack/fuzz/targets/direct.hpp"
#include "resmack/fuzz/trace.hpp"
#include "resmack/fuzz/trace_targets/fork.hpp"

extern "C" int __lsan_is_turned_off() { return 1; }

static resmack::Vector<resmack::fuzz::Tracer*> TRACERS;

void sigint_handler(int signum) {
  printf(
    "\nCaught signal %d on main process, terminating fuzzing procs\n",
    signum
  );
  for (resmack::fuzz::Tracer* tracer: TRACERS) {
    tracer->Stop();
  }
}

void PrintHelp(char* prog_name) {
  std::cout << resmack::GetResmackLogo() << std::endl;

  std::cout << prog_name << std::endl << std::endl;
  std::cout << "  Fuzz the compiled target" << std::endl << std::endl;
  std::cout << prog_name << " [-d MAX_DEPTH] [-n NPROCS] [-s] [-i INTERVAL] [--help]"  << std::endl << std::endl;
  std::cout << "          --help, -h            Show this help message" << std::endl;
  std::cout << "        --nprocs, -n NPROCS     Number of times to fork" << std::endl;
  std::cout << "       --crashes, -c CRASH_DIR  Where to store crashing inputs" << std::endl;
  std::cout << "     --max-depth, -d MAX_DEPTH  Maximum grammar depth during recursion" << std::endl;
  std::cout << "     --max-iters, -m MAX_ITERS  Maximum number of iterations" << std::endl;
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
  size_t max_iters;
  int show_stats;
  size_t stats_interval;
  char* crash_output;
};

bool ParseOptions(int argc, char**argv, FuzzOptions* opts) {
    static struct option long_options[] = {
      { "help", no_argument, &opts->help, 'h' },
      { "nprocs", required_argument, 0, 'n' },
      { "crashes", required_argument, 0, 'c' },
      { "max-depth", required_argument, 0, 'd' },
      { "max-iters", required_argument, 0, 'm' },
      { "show-stats", no_argument, &opts->show_stats, 's' },
      { "stats-interval", required_argument, 0, 'i' },
      { 0, 0, 0, 0 },
    };
    int opt_index = 0;

    while (true) {
      int c = getopt_long(argc, argv, "hsd:n:i:m:c:", long_options, &opt_index);
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
          opts->max_depth = strtoull(optarg, NULL, 10);
          break;
        case 'm':
          opts->max_iters = strtoull(optarg, NULL, 10);
          break;
        case 'n':
          opts->nprocs = atoi(optarg);
          break;
        case 'i':
          opts->stats_interval = atoi(optarg);
          break;
        case 'c':
          opts->crash_output = optarg;
          break;
      }
    }

    return true;
}

struct LoopPrintStatusArgs {
  resmack::fuzz::states::MmapState* state;
  int show_stats;
  bool should_run;
};

void* LoopPrintStatus(void* args_ptr) {
  LoopPrintStatusArgs* args = (LoopPrintStatusArgs*)args_ptr;
  resmack::fuzz::states::MmapState* state = args->state;
  bool show_stats = args->show_stats;
  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
  std::chrono::high_resolution_clock::time_point end;
  uint64_t start_iters = state->GetNumIterations();
  int sleep_amt = 1;
  resmack::fuzz::Corpus* corpus = state->GetCorpus();
  resmack::fuzz::StateStats* stats = state->GetStats();

  while (true) {
    sleep_amt++;
    for (int i = 0; i < sleep_amt; i++) {
      if (!args->should_run) {
        return NULL;
      }
      sleep(1);
    }
    if (!args->should_run) {
      return NULL;
    }
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

  return NULL;
}

void FuzzLoop(
  size_t rule_idx,
  resmack::Rules* rules,
  resmack::fuzz::Feedback* feedback,
  resmack::fuzz::State* state,
  resmack::fuzz::Corpus* corpus,
  FuzzOptions* opts,
  resmack::fuzz::Tracee* tracee
) {
  resmack::Rand* meta_rand = new resmack::Rand();;
  resmack::Rand* build_rand = new resmack::Rand(meta_rand->Next());
  build_rand->SetShouldRecord(true);

  std::string output;

  resmack::fuzz::DirectTarget target;
  resmack::fuzz::TargetSettings settings;
  resmack::fuzz::TargetStats stats(opts->stats_interval);
  resmack::BuildContext ctx(&output, build_rand, opts->max_depth);

  resmack::Vector<resmack::RandSnapshot> *mutated_replay =
    new resmack::Vector<resmack::RandSnapshot>();

  size_t counts = 0;
  while (opts->max_iters == 0 || state->GetNumIterations() < opts->max_iters) {
    counts++;
    if ((counts % opts->stats_interval) == 0) {
      state->IncNumIterations(opts->stats_interval);
      state->SyncStats(&stats);
      stats.Clear();
    }
    stats.Tick();

    size_t last_corpus_idx = 0;
    bool used_corpus = false;
    if (corpus->NumItems() > 0 && meta_rand->Maybe()) {
      used_corpus = true;
      resmack::Vector<resmack::RandSnapshot>* replay;
      RECORD_STAT(&stats, resmack::fuzz::SampleTypes::CORPUS, {
        replay = corpus->GetItem(meta_rand);
      });
      RECORD_STAT(&stats, resmack::fuzz::SampleTypes::MUTATE, {
        resmack::fuzz::MutateRandSnapshot(meta_rand, replay, mutated_replay);
      });
      ctx.SetReplay(mutated_replay);
    } else {
      ctx.SetReplay(NULL);
    }

    output.clear();
    build_rand->SnapshotClear();

    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::GENERATE, {
      rules->Build(rule_idx, &ctx);
    });

    // this occurs *in* the traced process (after forking/*).
    // If an exception occurs, these values are extracted and
    // used to save crash information and update corpus stats
    tracee->SaveLastCorpusInfo(used_corpus, last_corpus_idx, opts->max_depth);
    tracee->SaveLastReplay(mutated_replay);

    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::TARGET, {
      target.Launch(feedback, &output, &settings, &stats);
    });
    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::TARGET_RESET, {
      target.Reset();
    });
    size_t cov_key = feedback->GetStats().key;

    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::CORPUS, {
      if (corpus->AddRandSnapshotIfNotSeen(build_rand->GetSnapshots(), cov_key)) {
        //std::cout << "New coverage with: " << output << ", key: " << cov_key << ", num: " << feedback->GetStats().num << ", iters: " << counts << std::endl;
      }
    });
  }

  delete meta_rand;
  delete build_rand;
  delete mutated_replay;
}

bool HandleException(
  FuzzOptions* opts,
  resmack::Rules* rules,
  resmack::fuzz::State* state,
  size_t rule_idx,
  pid_t, // pid
  int, // status
  resmack::fuzz::Tracer* tracer,
  resmack::fuzz::Tracee* tracee
) {
  std::string output;
  // doesn't have to be the same one as before since we're doing a full,
  // unmodified replay
  resmack::Rand rand;
  resmack::Vector<resmack::RandSnapshot> snapshot;
  tracee->LoadLastReplay(&snapshot);

  resmack::BuildContext ctx(&output, &rand, tracee->GetLastMaxDepth());
  ctx.SetReplay(&snapshot);

  rules->Build(rule_idx, &ctx);

  resmack::fuzz::CrashInfo* info = tracer->GetCrashInfo();
  std::string out_path = std::string(opts->crash_output) + "/" + info->major_hash;
  if (!std::filesystem::exists(out_path)) {
    std::filesystem::create_directories(out_path);
  }
  out_path += "/";
  out_path += info->minor_hash;

  state->IncNumCrashesIfTrue([out_path, output]() -> bool {
    if (!std::filesystem::exists(out_path)) {
      std::ofstream file;
      file.open(out_path.c_str(), std::ofstream::out | std::ofstream::binary);
      file << output;
      file.close();
    } else {
      return false;
    }
  });

  // always restart the traced program
  return true;
}

__attribute__((visibility("default")))
int main(int argc, char** argv) {
  FuzzOptions opts {
    .help = false,
    .nprocs = 1,
    .max_depth = 10,
    .max_iters = 0,
    .show_stats = false,
    .stats_interval = 0x1000,
    .crash_output = "crashes"
  };
  signal(SIGINT, sigint_handler);

  ParseOptions(argc, argv, &opts);
  if (opts.help) {
    PrintHelp(argv[0]);
    return 1;
  }

  resmack::fuzz::ExternalFunctions EF;

  resmack::Rules rules = new resmack::Rules();
  size_t rule_idx = EF.ResmackGrammarInit(&rules);

  resmack::fuzz::Coverage cov;
  //resmack::fuzz::NoopCoverage noop_cov;
  char state_path[4096];
  snprintf(state_path, sizeof(state_path), "%s.resmack-state", argv[0]);
  resmack::fuzz::states::MmapState mmap_state(state_path);
  resmack::fuzz::Corpus* corpus = mmap_state.GetCorpus();

  bool is_main_proc = true;

  resmack::fuzz::trace_targets::Fork trace_target(
    [rule_idx, &rules, &cov, &mmap_state, corpus, &opts](resmack::fuzz::Tracee* tracee) {
      FuzzLoop(rule_idx, &rules, &cov, &mmap_state, corpus, &opts, tracee);
    }
  );

  size_t child_num;
  std::cout << "Creating " << opts.nprocs << " proceses for fuzzing" << std::endl;
  for (child_num = 0; child_num < opts.nprocs; child_num++ ) {
    resmack::fuzz::Tracer* tracer = new resmack::fuzz::Tracer(
      &trace_target,
      [&rules, rule_idx, &mmap_state, &opts](
        pid_t pid,
        int status,
        resmack::fuzz::Tracer* tracer,
        resmack::fuzz::Tracee* tracee
      ) -> bool {
        return HandleException(&opts, &rules, &mmap_state, rule_idx, pid, status, tracer, tracee);
      }
    );
    tracer->Trace();
    TRACERS.push_back(tracer);
  }

  pthread_t status_thread;
  LoopPrintStatusArgs status_args {
    .state = &mmap_state,
    .show_stats = opts.show_stats,
    .should_run = true,
  };
  if (is_main_proc) {
    pthread_create(&status_thread, NULL, LoopPrintStatus, (void*)&status_args);
  }

  for (resmack::fuzz::Tracer* tracer: TRACERS) {
    tracer->Join();
  }

  status_args.should_run = false;
  pthread_join(status_thread, NULL);
}
