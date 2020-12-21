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
#include <sched.h>
#include <semaphore.h>
#include <set>
#include <signal.h>
#include <string>
#include <thread>
#include <unistd.h>

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
#include "resmack/fuzz/tracee.hpp"
#include "resmack/fuzz/trace_targets/fork.hpp"
#include "resmack/fuzz/utils.hpp"

static bool SHUTTING_DOWN = false;

extern "C" {
  int __lsan_is_turned_off() { return 1; }

  void __sanitizer_cov_trace_pc_guard(uint32_t* guard_var) {
    if (SHUTTING_DOWN) { return; }
    resmack::fuzz::HandleSanitizerCovTracePcGuard(guard_var);
  }

  void __sanitizer_cov_trace_pc_guard_init(uint32_t* start, uint32_t* end) {
    resmack::fuzz::HandleSanitizerCovTracePcGuardInit(start, end);
  }
};

struct LoopPrintStatusArgs {
  resmack::fuzz::states::MmapState* state;
  resmack::fuzz::Feedback* feedback;
  int show_stats;
  bool should_run;
};

static resmack::Vector<resmack::fuzz::Tracer*> TRACERS;
pthread_t STATUS_THREAD;
LoopPrintStatusArgs STATUS_ARGS;

struct FuzzOptions {
  int help;
  size_t nprocs;
  size_t max_depth;
  size_t max_iters;
  size_t max_crashes;
  int show_stats;
  int mute_stdio;
  float print_interval;
  size_t stats_interval;
  char* crash_output;
  size_t create_threshhold;
  char* dump_corpus_path;
  uint32_t corpus_strats;
  int run_direct;
  char* state_path;
  bool pin_cpus;
  size_t corpus_decay;
};

static FuzzOptions OPTS {
  .help = false,
  .nprocs = 1,
  .max_depth = 10,
  .max_iters = 0,
  .max_crashes = 0,
  .show_stats = false,
  .mute_stdio = true,
  .print_interval = 0.0f,
  .stats_interval = 0x1000,
  .crash_output = (char*)"crashes",
  .create_threshhold = 100000,
  .dump_corpus_path = NULL,
  .corpus_strats = 0,
  .run_direct = 0,
  .state_path = (char*)"",
  .pin_cpus = true,
  .corpus_decay = 1000000, // 100,000 default decay
};

int MAIN_PID;

void sigint_handler(int signum) {
  if (::getpid() != MAIN_PID) { return; }

  SHUTTING_DOWN = true;
  printf(
    "\nCaught signal %d on main process, terminating fuzzing procs\n",
    signum
  );

  for (resmack::fuzz::Tracer* tracer: TRACERS) {
    tracer->Stop();
  }
  
  char sem_path[2 + (SHA_DIGEST_LENGTH * 2)];

  resmack::fuzz::utils::sha1_hex(OPTS.state_path, strlen(OPTS.state_path), sem_path+1);
  sem_path[0] = '/';
  if(sem_unlink(sem_path) != 0) {
    // doesn't exist, so no problem!
  }

  char corpus_path[4096];
  snprintf(corpus_path, sizeof(corpus_path), "%s-corpus", OPTS.state_path);
  resmack::fuzz::utils::sha1_hex(corpus_path, strlen(corpus_path), sem_path+1);
  sem_path[0] = '/';
  if(sem_unlink(sem_path) != 0) {
    // doesn't exist, so no problem!
  }

  STATUS_ARGS.should_run = false;
}

void PrintHelp(char* prog_name) {
  std::cout << resmack::GetResmackLogo() << std::endl;

  std::cout << prog_name << std::endl << std::endl;
  std::cout << "  Fuzz the compiled target" << std::endl << std::endl;
  std::cout << prog_name << " [-d MAX_DEPTH] [-n NPROCS] [-s] [-i INTERVAL] [--help]"  << std::endl << std::endl;
  std::cout << "             --help,-h             Show this help message" << std::endl;
  std::cout << "           --direct                Run the fuzzing loop and target in the main thread (" << OPTS.run_direct << ")" << std::endl;
  std::cout << "           --nprocs,-n NPROCS      Number of times to fork (" << OPTS.nprocs << ")" << std::endl;
  std::cout << "          --crashes,-c CRASH_DIR   Where to store crashing inputs (" << OPTS.crash_output << ")" << std::endl;
  std::cout << "          --no-mute                Do not mute stdio of fuzz procs (" << !OPTS.mute_stdio << ")" << std::endl;
  std::cout << "        --max-depth,-d MAX_DEPTH   Maximum grammar depth during recursion (" << OPTS.max_depth << ")" << std::endl;
  std::cout << "        --max-iters,-m MAX_ITERS   Maximum number of iterations (" << OPTS.max_iters << ")" << std::endl;
  std::cout << "       --no-pin-cpu                Don't pin processes to specific CPUs (" << !OPTS.pin_cpus << ")" << std::endl;
  std::cout << "      --max-crashes,-M MAX_CRASHES Maximum number of crashes (" << OPTS.max_crashes << ")" << std::endl;
  std::cout << "       --show-stats,-s             Show stat percentages (" << OPTS.show_stats << ")" << std::endl;
  std::cout << "       --state-path,-S             Path to where the state file should be stored (" << OPTS.state_path << ")" << std::endl;
  std::cout << "      --dump-corpus,-D DIR         Dump the corpus to DIR" << std::endl;
  std::cout << "     --corpus-decay                The number of mutation attempts that causes a" << std::endl;
  std::cout << "                                   corpus entry to be fully deprioritized (" << OPTS.corpus_decay << ")" << std::endl;
  std::cout << "     --corpus-strat,-C STRAT       Enable the corpus strategy STRAT, must be one of below options." << std::endl;
  std::cout << "                                   May be used multiple times:" << std::endl;
  std::cout << "                                      MOST_FEEDBACK            MOST_ANCESTORS   MOST_DESCENDANTS" << std::endl;
  std::cout << "                                     LEAST_FEEDBACK           LEAST_ANCESTORS  LEAST_DESCENDANTS" << std::endl;
  std::cout << "                                        MOST_RECENT   MOST_DIRECT_DESCENDANTS               RAND" << std::endl;
  std::cout << "                                       LEAST_RECENT  LEAST_DIRECT_DESCENDANTS" << std::endl;
  std::cout << "                                       LEAST_RECENT  LEAST_DIRECT_DESCENDANTS" << std::endl;
  std::cout << "   --stats-interval,-i INTERVAL    Collect stats on every Nth iteration (" << OPTS.stats_interval << ")" << std::endl;
  std::cout << "   --print-interval,-p INTERVAL    Print at set intervals with no backoff (" << OPTS.print_interval << ")" << std::endl;
  std::cout << "--create-threshhold,-t THRESHOLD   The threshold to create new inputs (" << OPTS.create_threshhold << ")" << std::endl;
  std::cout << std::endl;
  std::cout << "Example:" << std::endl << std::endl;
  std::cout << prog_name << " -n 3 --show-stats" << std::endl;
}

bool ParseOptions(int argc, char**argv) {
#define OPT_NO_MUTE      1000
#define OPT_RUN_DIRECT   1001
#define OPT_NO_PIN_CPU   1002
#define OPT_CORPUS_DECAY 1003
    static struct option long_options[] = {
      { "help", no_argument, 0, 'h' },
      { "nprocs", required_argument, 0, 'n' },
      { "crashes", required_argument, 0, 'c' },
      { "direct", no_argument, 0, OPT_RUN_DIRECT },
      { "no-mute", no_argument, 0, OPT_NO_MUTE },
      { "max-depth", required_argument, 0, 'd' },
      { "max-iters", required_argument, 0, 'm' },
      { "max-crashes", required_argument, 0, 'M' },
      { "show-stats", no_argument, &OPTS.show_stats, 's' },
      { "state-path", required_argument, 0, 'S' },
      { "corpus-strat", required_argument, 0, 'C' },
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
    OPTS.state_path = state_path;

    while (true) {
      int c = getopt_long(argc, argv, "hsd:n:i:m:c:t:D:S:p:M:C:", long_options, &opt_index);
      if (c == -1) {
        break;
      }

      switch (c) {
        case 0:
          break;
        case 'h':
          OPTS.help = true;
          break;
        case 's':
          OPTS.show_stats = true;
          break;
        case 'd':
          OPTS.max_depth = strtoull(optarg, NULL, 10);
          break;
        case 'm':
          OPTS.max_iters = strtoull(optarg, NULL, 10);
          break;
        case 'M':
          OPTS.max_crashes = strtoull(optarg, NULL, 10);
          break;
        case 'n':
          OPTS.nprocs = strtoull(optarg, NULL, 10);
          break;
        case 'i':
          OPTS.stats_interval = strtoull(optarg, NULL, 10);
          break;
        case 'c':
          OPTS.crash_output = optarg;
          break;
        case 't':
          OPTS.create_threshhold = strtoull(optarg, NULL, 10);
          break;
        case 'D':
          OPTS.dump_corpus_path = optarg;
          break;
        case OPT_NO_MUTE:
          OPTS.mute_stdio = false;
          break;
        case OPT_RUN_DIRECT:
          OPTS.run_direct = true;
          break;
        case 'S':
          OPTS.state_path = optarg;
          break;
        case 'C':
          if (strcmp(optarg, "MOST_FEEDBACK") == 0) {
            OPTS.corpus_strats |= resmack::fuzz::corpora::STRAT_MOST_FEEDBACK;
          } else if (strcmp(optarg, "LEAST_FEEDBACK") == 0) {
            OPTS.corpus_strats |= resmack::fuzz::corpora::STRAT_LEAST_FEEDBACK;
          } else if (strcmp(optarg, "MOST_RECENT") == 0) {
            OPTS.corpus_strats |= resmack::fuzz::corpora::STRAT_MOST_RECENT;
          } else if (strcmp(optarg, "LEAST_RECENT") == 0) {
            OPTS.corpus_strats |= resmack::fuzz::corpora::STRAT_LEAST_RECENT;
          } else if (strcmp(optarg, "MOST_ANCESTORS") == 0) {
            OPTS.corpus_strats |= resmack::fuzz::corpora::STRAT_MOST_ANCESTORS;
          } else if (strcmp(optarg, "LEAST_ANCESTORS") == 0) {
            OPTS.corpus_strats |= resmack::fuzz::corpora::STRAT_LEAST_ANCESTORS;
          } else if (strcmp(optarg, "MOST_DIRECT_DESCENDANTS") == 0) {
            OPTS.corpus_strats |= resmack::fuzz::corpora::STRAT_MOST_DIRECT_DESCENDANTS;
          } else if (strcmp(optarg, "LEAST_DIRECT_DESCENDANTS") == 0) {
            OPTS.corpus_strats |= resmack::fuzz::corpora::STRAT_LEAST_DIRECT_DESCENDANTS;
          } else if (strcmp(optarg, "MOST_DESCENDANTS") == 0) {
            OPTS.corpus_strats |= resmack::fuzz::corpora::STRAT_MOST_DESCENDANTS;
          } else if (strcmp(optarg, "LEAST_DESCENDANTS") == 0) {
            OPTS.corpus_strats |= resmack::fuzz::corpora::STRAT_LEAST_DESCENDANTS;
          } else if (strcmp(optarg, "RAND") == 0) {
            OPTS.corpus_strats |= resmack::fuzz::corpora::STRAT_RAND;
          } else {
            std::cout << "Invalid corpus strategy: " << optarg << std::endl;
            std::exit(1);
          }
          break;
        case 'p':
          OPTS.print_interval = std::stof(optarg);
          break;
        case OPT_NO_PIN_CPU:
          OPTS.pin_cpus = false;
          break;
        case OPT_CORPUS_DECAY:
          OPTS.corpus_decay = strtoull(optarg, NULL, 10);
          break;
      }
    }

    return true;
}

void PrintStatus(
  LoopPrintStatusArgs* args,
  std::chrono::high_resolution_clock::time_point start,
  uint64_t start_iters
) {
  std::chrono::high_resolution_clock::time_point end;
  resmack::fuzz::states::MmapState* state = args->state;
  bool show_stats = args->show_stats;

  resmack::fuzz::corpora::MmapCorpus* corpus = state->GetMmapCorpus();
  resmack::fuzz::StateStats* stats = state->GetStats();

  end = std::chrono::high_resolution_clock::now();
  uint64_t num_iters = state->GetNumIterations();
  uint64_t session_iters = num_iters - start_iters;
  std::chrono::duration<double> span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
  printf(
    "Iters: %lu | %0.2f iters/s | Crashes: %lu | Corpus: %lu (D:%05.2f) | Feedback: %s | %0.2f s\n",
    num_iters,
    (float)session_iters / span.count(),
    state->GetNumCrashes(),
    corpus->NumItemsRaw(),
    corpus->GetDecayPercent(),
    args->feedback->GetSummary().c_str(),
    span.count()
  );

  if (!show_stats) {
    return;
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

void* LoopPrintStatus(void* args_ptr) {
  LoopPrintStatusArgs* args = (LoopPrintStatusArgs*)args_ptr;
  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
  resmack::fuzz::states::MmapState* state = args->state;
  uint64_t start_iters = state->GetNumIterations();

  size_t sleep_amt;
  if (OPTS.print_interval != 0.0f) {
    sleep_amt = (size_t)(OPTS.print_interval * 1000);
  } else {
    sleep_amt  = 1 * 1000; // ms
  }

  while (args->should_run) {
    if (OPTS.print_interval == 0.0f) {
      sleep_amt += 1000;
    }
    size_t total_slept = 0;
    size_t increment = 50;
    size_t prev_corpus_count = state->GetCorpus()->NumItemsRaw();
    size_t prev_crash_count = state->GetNumCrashes();
    while(total_slept < sleep_amt) {
      if (!args->should_run) {
        break;
      }
      if (OPTS.print_interval == 0.0f && (
        state->GetCorpus()->NumItemsRaw() != prev_corpus_count
        || state->GetNumCrashes() != prev_crash_count
      )) {
          break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(increment));
      total_slept += increment;
    }
    if (!args->should_run) {
      break;
    }
    // in case we broke early to show immediate status
    sleep_amt -= (sleep_amt - total_slept);

    PrintStatus(args, start, start_iters);
  }
  // one final status print
  PrintStatus(args, start, start_iters);

  return NULL;
}

void FuzzLoop(
  size_t rule_idx,
  resmack::Rules* rules,
  resmack::fuzz::Feedback* feedback,
  resmack::fuzz::State* state,
  resmack::fuzz::Corpus* corpus,
  resmack::fuzz::Tracee* tracee
) {
  resmack::Rand meta_rand;
  resmack::Rand build_rand(meta_rand.Next());
  build_rand.SetShouldRecord(true);

  std::string output;

  resmack::fuzz::DirectTarget target;
  resmack::fuzz::TargetSettings settings;
  resmack::fuzz::TargetStats stats(OPTS.stats_interval);
  resmack::BuildContext ctx(&output, &build_rand, OPTS.max_depth);

  std::this_thread::sleep_for(std::chrono::milliseconds(meta_rand.Next() & 0xff));

  resmack::Vector<resmack::RandSnapshot> mutated_replay;

  corpus->Sync();

  size_t counts = 0;
  bool past_create_threshhold = true;
  while (
      (OPTS.max_iters == 0 || state->GetNumIterations() < OPTS.max_iters) &&
      (OPTS.max_crashes == 0 || state->GetNumCrashes() < OPTS.max_crashes)
  ) {
    counts++;
    if ((counts % OPTS.stats_interval) == 0) {
      state->IncNumIterations(OPTS.stats_interval);
      state->SyncStats(&stats);
      stats.Clear();
      bool tmp = (corpus->ItersSinceNewItem() > OPTS.create_threshhold);
      if (tmp && !past_create_threshhold) {
        meta_rand.ReinitSeed();
        build_rand.ReinitSeed();
      }
      past_create_threshhold = tmp;
    }
    stats.Tick();

    size_t last_corpus_idx = 0;
    bool used_corpus = false;
    if (corpus->NumItems() == 0 || (past_create_threshhold && meta_rand.Maybe())) {
      ctx.SetReplay(NULL);
      ctx.max_depth = meta_rand.NextInRangeGaussian(1, OPTS.max_depth);
      //ctx.max_depth = (meta_rand.Next() % (OPTS.max_depth - 1)) + 1;
    } else {
      used_corpus = true;
      resmack::Vector<resmack::RandSnapshot>* replay;
      RECORD_STAT(&stats, resmack::fuzz::SampleTypes::CORPUS, {
        replay = corpus->GetItem(&meta_rand);
      });
      RECORD_STAT(&stats, resmack::fuzz::SampleTypes::MUTATE, {
        resmack::fuzz::MutateRandSnapshot(
          &meta_rand,
          replay,
          &mutated_replay,
          OPTS.max_depth
        );
      });
      ctx.SetReplay(&mutated_replay);
    }

    output.clear();
    build_rand.SnapshotClear();

    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::GENERATE, {
      rules->Build(rule_idx, &ctx);
    });

    //output = "I mock apples and bananas and grapes or bananas or apples without peaches without bananas with peaches or pears and apples and bananas";

    // this occurs *in* the traced process (after forking/*).
    // If an exception occurs, these values are extracted and
    // used to save crash information and update corpus stats
    if (tracee != NULL) {
      tracee->SaveLastCorpusInfo(used_corpus, last_corpus_idx, OPTS.max_depth);
      tracee->SaveLastReplay(&mutated_replay);
    }

    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::TARGET, {
      target.Launch(feedback, &output, &settings, &stats);
    });
    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::TARGET_RESET, {
      target.Reset();
    });
    resmack::fuzz::FeedbackStats feedback_stats = feedback->GetStats();

    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::CORPUS, {
      if (feedback_stats.new_coverage && corpus->AddRandSnapshotIfNotSeen(build_rand.GetSnapshots(), feedback_stats, used_corpus)) {
        past_create_threshhold = false;
        std::cout << "New coverage with: " << output << ", " << ", iters: " << counts << std::endl;
      }
    });
  }
}

bool HandleException(
  resmack::Rules* rules,
  resmack::fuzz::State* state,
  size_t rule_idx,
  pid_t, // pid
  int, // status
  resmack::fuzz::Tracer* tracer,
  resmack::fuzz::Tracee* tracee
) {
  if (SHUTTING_DOWN) {
    return false;
  }
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
  std::string out_path = std::string(OPTS.crash_output) + "/" + info->major_hash;
  if (!std::filesystem::exists(out_path)) {
    std::filesystem::create_directories(out_path);
  }
  out_path += "/";
  out_path += info->minor_hash;

  state->IncNumCrashesIfTrue([out_path, output, tracee]() -> bool {
    if (SHUTTING_DOWN) {
      return false;
    }
    if (!std::filesystem::exists(out_path)) {
      std::ofstream file;
      file.open(out_path.c_str(), std::ofstream::out | std::ofstream::binary);
      file.write(output.data(), output.size());
      file.close();

      resmack::fuzz::ser::AsanInfo* asan_info = tracee->GetAsanInfo();
      if (asan_info != NULL) {
        std::string asan_path = out_path + ".asan.txt";
        file.open(asan_path.c_str(), std::ofstream::out | std::ofstream::binary);
        file.write(asan_info->report, strlen(asan_info->report));
        file.close();
      }
      return true;
    } else {
      return false;
    }
  });

  // always restart the traced program
  if (OPTS.max_crashes != 0 && state->GetNumCrashes() >= OPTS.max_crashes) {
    // this only applies to *THIS* forked process - we need to stop
    // capturing coverage data
    SHUTTING_DOWN = true;
    return false;
  }
  return true;
}

void DumpCorpus(resmack::Rules* rules, resmack::fuzz::Corpus* corpus, size_t rule_idx) {
  resmack::Rand rand;
  std::string output;
  resmack::BuildContext ctx(&output, &rand, OPTS.max_depth);

  if (!std::filesystem::exists(OPTS.dump_corpus_path)) {
    std::filesystem::create_directories(OPTS.dump_corpus_path);
  }

  const resmack::Vector<resmack::fuzz::CorpusEntry>* corpus_items = corpus->GetItems();

  size_t i;
  for (i = 0; i < corpus_items->size(); i++) {
    std::cout << ".";
    const resmack::fuzz::CorpusEntry* entry = &corpus_items->at(i);

    output.clear();
    ctx.SetReplay(&entry->snapshot);
    rules->Build(rule_idx, &ctx);

    char* output_sha = resmack::fuzz::utils::sha1_hex(output.data(), output.size(), NULL);
    std::string out_path = std::string(OPTS.dump_corpus_path) + "/" + output_sha;
    free(output_sha);

    if (std::filesystem::exists(out_path)) { continue; }

    std::ofstream file;
    file.open(out_path.c_str(), std::ofstream::out | std::ofstream::binary);
    file.write(output.data(), output.size());
    file.close();
  }
  std::cout << std::endl;

  std::cout << "Wrote " << i << " corpus entries to " << OPTS.dump_corpus_path << std::endl;
}

__attribute__((visibility("default")))
int main(int argc, char** argv) {
  MAIN_PID = getpid();

  ParseOptions(argc, argv);
  if (OPTS.help) {
    PrintHelp(argv[0]);
    return 1;
  }

  resmack::fuzz::ExternalFunctions EF;

  resmack::Rules rules;
  if (EF.ResmackGrammarInit == NULL) {
    std::cout << "ResmackGrammarInit is not defined" << std::endl;
    return 1;
  }
  size_t rule_idx = EF.ResmackGrammarInit(&rules);
  rules.Finalize();

  resmack::fuzz::states::MmapState mmap_state(OPTS.state_path);
  resmack::fuzz::corpora::MmapCorpus* corpus = mmap_state.GetMmapCorpus();
  corpus->SetCorpusDecay(OPTS.corpus_decay);
  resmack::fuzz::Coverage cov;
  //resmack::fuzz::NoopCoverage noop_cov;
  
  if (OPTS.corpus_strats != 0) {
    reinterpret_cast<resmack::fuzz::corpora::MmapCorpus*>(corpus)->SetStrats(OPTS.corpus_strats);
  }

  if (OPTS.dump_corpus_path) {
    DumpCorpus(&rules, corpus, rule_idx);
    return 0;
  }

  if (OPTS.run_direct) {
    FuzzLoop(rule_idx, &rules, &cov, &mmap_state, corpus, NULL);
    std::exit(0);
  }

  //may return 0 when not able to detect
  const auto nproc = std::thread::hardware_concurrency();

  resmack::fuzz::trace_targets::Fork trace_target(
    OPTS.mute_stdio,
    [nproc, rule_idx, &rules, &cov, &mmap_state, corpus](resmack::fuzz::Tracee* tracee) {
      uint32_t idx = tracee->GetIdx();

      // just in case the number of forks is set to be higher than
      // nproc
      if (OPTS.pin_cpus && idx < nproc) {
        cpu_set_t cpus;
        CPU_ZERO(&cpus);
        CPU_SET(idx, &cpus);
        sched_setaffinity(0, sizeof(cpu_set_t), &cpus);
      }

      FuzzLoop(rule_idx, &rules, &cov, &mmap_state, corpus, tracee);
      SHUTTING_DOWN = true;
    }
  );

  size_t child_num;
  std::cout << "Creating " << OPTS.nprocs << " processes for fuzzing" << std::endl;
  for (child_num = 0; child_num < OPTS.nprocs; child_num++ ) {
    resmack::fuzz::Tracer* tracer = new resmack::fuzz::Tracer(
      &trace_target,
      [&rules, rule_idx, &mmap_state](
        pid_t pid,
        int status,
        resmack::fuzz::Tracer* tracer,
        resmack::fuzz::Tracee* tracee
      ) -> bool {
        return HandleException(&rules, &mmap_state, rule_idx, pid, status, tracer, tracee);
      },
      child_num
    );
    tracer->Trace();
    TRACERS.push_back(tracer);
  }

  STATUS_ARGS.state = &mmap_state;
  STATUS_ARGS.show_stats = OPTS.show_stats;
  STATUS_ARGS.should_run = true;
  STATUS_ARGS.feedback = &cov;

  pthread_create(&STATUS_THREAD, NULL, LoopPrintStatus, (void*)&STATUS_ARGS);
  signal(SIGINT, sigint_handler);

  for (resmack::fuzz::Tracer* tracer: TRACERS) {
    tracer->Join();
  }

  SHUTTING_DOWN = true;

  STATUS_ARGS.should_run = false;
  pthread_join(STATUS_THREAD, NULL);
}
