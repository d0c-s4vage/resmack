#include <chrono>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <openssl/sha.h>
#include <pthread.h>
#include <filesystem>
#include <sched.h>
#include <semaphore.h>
#include <csignal>
#include <string>
#include <thread>
#include <unistd.h>

#include "resmack/fuzz/asan_util.hpp"
#include "resmack/logo.hpp"
#include "resmack/build_context.hpp"
#include "resmack/rand.hpp"
#include "resmack/types.hpp"
#include "resmack/rules.hpp"
#include "resmack/debug.hpp"

#include "resmack/fuzz/corpus.hpp"
#include "resmack/fuzz/external.hpp"
#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/feedbacks/coverage.hpp"
#include "resmack/fuzz/lock.hpp"
#include "resmack/fuzz/mutate.hpp"
#include "resmack/fuzz/state.hpp"
#include "resmack/fuzz/states/mmap.hpp"
#include "resmack/fuzz/targets/direct.hpp"
#include "resmack/fuzz/tracer.hpp"
#include "resmack/fuzz/tracee.hpp"
#include "resmack/fuzz/process_launchers/fork.hpp"
#include "resmack/fuzz/utils.hpp"

namespace fs = std::filesystem;

namespace  {

struct LoopPrintStatusArgs {
  resmack::fuzz::states::MmapState* state;
  resmack::fuzz::Feedback* feedback;
  bool show_stats;
};

resmack::fuzz::Lock io_lock("ResmackIoLock", true);

resmack::Vector<resmack::fuzz::Tracer*> TRACERS;
pthread_t STATUS_THREAD = 0;
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
  fs::path crash_output;
  size_t create_threshhold;
  fs::path dump_corpus_path;
  bool do_corpus_dump;
  uint32_t corpus_strats;
  int run_direct;
  fs::path state_path;
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
  .stats_interval = 10000,
  .crash_output = fs::absolute(fs::path("crashes")),
  .create_threshhold = 100000,
  // is ignored until set by the user
  .dump_corpus_path = fs::absolute(fs::path("/dev/null")),
  .do_corpus_dump = false,
  .corpus_strats = 0,
  .run_direct = 0,
  // is set explicitly in ParseOptions
  .state_path = fs::absolute(fs::path(".resmack-state")),
  .pin_cpus = true,
  .corpus_decay = 100'000, // 100,000 default decay
};

pid_t MAIN_PID;
std::atomic<bool> SHUTTING_DOWN(false);

void sigint_handler([[maybe_unused]] int signum) {
  if (getpid() != MAIN_PID) { return; }
  DEBUG_PRINT("SIGINT handler from main process\n");
  SHUTTING_DOWN.store(true);
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

    OPTS.state_path = fs::absolute(fs::path(std::string(argv[0]) + ".resmack-state"));

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
          OPTS.max_depth = strtoull(optarg, nullptr, 10);
          break;
        case 'm':
          OPTS.max_iters = strtoull(optarg, nullptr, 10);
          break;
        case 'M':
          OPTS.max_crashes = strtoull(optarg, nullptr, 10);
          break;
        case 'n':
          OPTS.nprocs = strtoull(optarg, nullptr, 10);
          break;
        case 'i':
          OPTS.stats_interval = strtoull(optarg, nullptr, 10);
          break;
        case 'c':
          OPTS.crash_output = fs::absolute(fs::path(optarg));
          break;
        case 't':
          OPTS.create_threshhold = strtoull(optarg, nullptr, 10);
          break;
        case 'D':
          OPTS.dump_corpus_path = fs::absolute(fs::path(optarg));
          OPTS.do_corpus_dump = true;
          break;
        case OPT_NO_MUTE:
          OPTS.mute_stdio = false;
          break;
        case OPT_RUN_DIRECT:
          OPTS.run_direct = true;
          break;
        case 'S':
          OPTS.state_path = fs::absolute(fs::path(optarg));
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
          OPTS.corpus_decay = strtoull(optarg, nullptr, 10);
          break;
      }
    }

    return true;
}

void PrintStatus(
  resmack::fuzz::states::MmapState* state,
  resmack::fuzz::Feedback* feedback,
  bool show_stats,
  float start,
  uint64_t start_iters
) {
  std::cout << std::flush;

  resmack::fuzz::corpora::MmapCorpus* corpus = state->GetMmapCorpus();
  feedback->Sync();
  corpus->Sync();

  resmack::fuzz::StateStats* stats = state->GetStats();

  uint64_t num_iters = state->GetNumIterations();
  uint64_t session_iters = num_iters - start_iters;
  float span = resmack::fuzz::utils::GetTimeNow() - start;
  /*
  printf(
    "Iters: %lu | %0.2f iters/s | Crashes: %lu | Corpus: %lu | Feedback: %s | %0.2f s\n",
    num_iters,
    (float)session_iters / span,
    state->GetNumCrashes(),
    corpus->NumItemsRaw(),
    feedback->GetSummary().c_str(),
    span,
  );
  */
  printf(
    "Iters: %lu | %0.2f iters/s | Crashes: %lu | Corpus: %lu (D:%05.2f,C:%05.2f) | Feedback: %s | %0.2f s\n",
    num_iters,
    (float)session_iters / span,
    state->GetNumCrashes(),
    corpus->NumItemsRaw(),
    corpus->GetDecayPercent(),
    corpus->GetUsedCapacity() * 100,
    feedback->GetSummary().c_str(),
    span
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

void LoopPrintStatus(
    resmack::fuzz::states::MmapState* state,
    resmack::fuzz::Feedback* feedback,
    bool show_stats
) {
  float start = resmack::fuzz::utils::GetTimeNow();
  float last_print = start;
  uint64_t start_iters = state->GetNumIterations();

  float sleep_amt;
  if (OPTS.print_interval != 0.0f) {
    sleep_amt = (float)(OPTS.print_interval);
  } else {
    sleep_amt  = 1.0f;
  }

  size_t increment = 50;
  size_t prev_corpus_count = state->GetCorpus()->NumItemsRaw();
  size_t prev_crash_count = state->GetNumCrashes();

  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(increment));

    float now = resmack::fuzz::utils::GetTimeNow();

    size_t curr_corpus_count = state->GetCorpus()->NumItemsRaw();
    size_t curr_crash_count = state->GetNumCrashes();
    if ((now - last_print) >= sleep_amt
      || curr_corpus_count != prev_corpus_count
      || curr_crash_count != prev_crash_count
    ) {
      PrintStatus(state, feedback, show_stats, start, start_iters);
      last_print = resmack::fuzz::utils::GetTimeNow();
    }

    prev_crash_count = curr_crash_count;
    prev_corpus_count = curr_corpus_count;
  } while(!SHUTTING_DOWN.load());
}

void* DoLoopPrintStatus(void* args_ptr) {
  LoopPrintStatusArgs* args = static_cast<LoopPrintStatusArgs*>(args_ptr);
  LoopPrintStatus(
      args->state,
      args->feedback,
      args->show_stats
  );
  return nullptr;
}

void FuzzLoop(
  size_t rule_idx,
  resmack::Rules* rules,
  resmack::fuzz::Feedback* feedback,
  resmack::fuzz::State* state,
  resmack::fuzz::Corpus* corpus,
  resmack::fuzz::Tracee* tracee
) {
  resmack::fuzz::ExternalFunctions EF;

  if (EF.ResmackInit != nullptr) {
    EF.ResmackInit();
  }

  resmack::Rand meta_rand;
  resmack::Rand build_rand(meta_rand.Next());
  build_rand.SetShouldRecord(true);

  std::string output;

  resmack::fuzz::DirectTarget target;
  resmack::fuzz::TargetSettings settings;
  resmack::fuzz::TargetStats stats(OPTS.stats_interval);
  resmack::BuildContext ctx(&output, &build_rand, OPTS.max_depth);

  // prevent a thundering herd
  std::this_thread::sleep_for(std::chrono::milliseconds(meta_rand.Next() & 0xfff));

  resmack::Vector<resmack::RandSnapshot> mutated_replay;

  corpus->Sync();
  feedback->Sync();

  size_t counts = 0;
  bool past_create_threshhold = true;
  while (!SHUTTING_DOWN.load(std::memory_order_relaxed)) {
    if (
        (OPTS.max_iters > 0 && state->GetNumIterations() >= OPTS.max_iters) ||
        (OPTS.max_crashes > 0 && state->GetNumCrashes() >= OPTS.max_crashes)
    ) {
      SHUTTING_DOWN.store(true, std::memory_order_relaxed);
      break;
    }

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

    size_t last_corpus_idx1 = 0;
    size_t last_corpus_idx2 = 0;
    bool used_corpus = false;
    if (corpus->NumItems() == 0 || (past_create_threshhold && meta_rand.Maybe())) {
      ctx.SetReplay(nullptr);
      ctx.max_depth = meta_rand.NextInRangeGaussian(1, OPTS.max_depth);
      //ctx.max_depth = (meta_rand.Next() % (OPTS.max_depth - 1)) + 1;
    } else {
      used_corpus = true;
      resmack::Vector<resmack::RandSnapshot>* replay;
      RECORD_STAT(&stats, resmack::fuzz::SampleTypes::CORPUS, {
        replay = corpus->GetItem(&meta_rand, &last_corpus_idx1, &last_corpus_idx2);
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

    // this occurs *in* the traced process (after forking/*).
    // If an exception occurs, these values are extracted and
    // used to save crash information and update corpus stats
    tracee->SaveLastCorpusInfo(used_corpus, last_corpus_idx1, last_corpus_idx2, OPTS.max_depth);
    tracee->SaveLastReplay(&mutated_replay);
    tracee->IterStart();

    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::TARGET, {
      target.Launch(feedback, &output, &settings, &stats);
    });

    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::TARGET_RESET, {
      target.Reset();
    });

    resmack::fuzz::FeedbackStats feedback_stats = feedback->GetStats();
    if (feedback_stats.new_coverage) {
      std::cout << "NEW COVERAGE: " << output << std::endl;
      feedback->Sync();
    }

    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::CORPUS, {
      if (stats.valid && feedback_stats.new_coverage) {
        if (corpus->AddRandSnapshotIfNotSeen(build_rand.GetSnapshots(), feedback_stats, used_corpus)) {
          past_create_threshhold = false;
        }
      }
    });
  }
  std::cout << std::flush;
}

void HandleTimeout(
  [[maybe_unused]] resmack::Rules* rules,
  resmack::fuzz::State* state,
  [[maybe_unused]] size_t rule_idx,
  [[maybe_unused]] pid_t pid, // pid
  [[maybe_unused]] resmack::fuzz::Tracer* tracer,
  resmack::fuzz::Tracee* tracee
) {
  if (SHUTTING_DOWN.load()) {
    return;
  }

  // nothing to do here
  if (!tracee->GetLastUsedCorpus()) {
    return;
  }
  state->IncNumIterations(1);

  // save the current state that generated the timeout
  // so that we don't do it again
  resmack::fuzz::Corpus* corpus = state->GetCorpus();
  corpus->Sync();
  corpus->IncUnwanted(tracee->GetLastCorpusIndex1());
  corpus->IncUnwanted(tracee->GetLastCorpusIndex2());
}

void HandleException(
  resmack::Rules* rules,
  resmack::fuzz::State* state,
  size_t rule_idx,
  [[maybe_unused]] pid_t pid, // pid
  resmack::fuzz::Tracer* tracer,
  resmack::fuzz::Tracee* tracee
) {
  if (SHUTTING_DOWN.load()) {
    return;
  }

  state->IncNumIterations(1);
  resmack::fuzz::Corpus* corpus = state->GetCorpus();
  corpus->IncUnwanted(tracee->GetLastCorpusIndex1());
  corpus->IncUnwanted(tracee->GetLastCorpusIndex2());
  corpus->Sync();

  std::string output;
  // doesn't have to be the same one as before since we're doing a full,
  // unmodified replay
  resmack::Rand rand;
  resmack::Vector<resmack::RandSnapshot> snapshot;
  tracee->LoadLastReplay(&snapshot);

  resmack::BuildContext ctx(&output, &rand, tracee->GetLastMaxDepth());
  ctx.SetReplay(&snapshot);

  rules->Build(rule_idx, &ctx);
  std::cout << "NEW COVERAGE (CRASH): " << output << std::endl;

  const resmack::fuzz::CrashInfo* info = tracer->GetCrashInfo();
  std::string out_path = std::string(OPTS.crash_output) + "/" + info->major_hash;
  if (!std::filesystem::exists(out_path)) {
    std::filesystem::create_directories(out_path);
  }
  out_path += "/";
  out_path += info->minor_hash;

  state->IncNumCrashesIfTrue([out_path, output, tracee, info]() -> bool {
    if (SHUTTING_DOWN.load()) {
      return false;
    }
    if (!std::filesystem::exists(out_path)) {
      std::ofstream file;
      file.open(out_path.c_str(), std::ofstream::out | std::ofstream::binary);
      file.write(output.data(), output.size());
      file.close();

      const resmack::fuzz::ser::AsanInfo* asan_info = tracee->GetAsanInfo();
      if (asan_info != nullptr) {
        std::string asan_path = out_path + ".asan.txt";
        file.open(asan_path.c_str(), std::ofstream::out | std::ofstream::binary);
        file.write(asan_info->report, strlen(asan_info->report));
        file.close();
      }

      std::string stack_path = out_path + ".stack.txt";

      std::string stack_data;
      stack_data += strsignal(info->signal_info.term_signal ? info->signal_info.term_signal : info->signal_info.stop_signal);
      stack_data += "\n";
      stack_data += info->minor_stack;
      stack_data += "\n";

      file.open(stack_path, std::ofstream::out | std::ofstream::binary);
      file.write(stack_data.data(), stack_data.size());
      file.close();
      return true;
    } else {
      return false;
    }
  });

  // always restart the traced program
  if (OPTS.max_crashes != 0 && state->GetNumCrashes() >= OPTS.max_crashes) {
    // this only applies to *THIS* forked process - we need to stop
    // capturing coverage data
    SHUTTING_DOWN.store(true);
    return;
  }
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

    char* output_sha = resmack::fuzz::utils::sha1_hex(output.data(), output.size(), nullptr);
    std::string out_path = std::string(OPTS.dump_corpus_path) + "/" + std::to_string(i) + "_" + output_sha;
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

} // namespace resmack::main

__attribute__((visibility("default")))
int main(int argc, char** argv) {
  MAIN_PID = getpid();
  DEBUG_PRINT("MAIN PID: %d\n", getpid());

  ParseOptions(argc, argv);
  if (OPTS.help) {
    PrintHelp(argv[0]);
    return 1;
  }

  resmack::fuzz::ExternalFunctions EF;

  resmack::Rules rules;
  if (EF.ResmackGrammarInit == nullptr) {
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

  if (OPTS.do_corpus_dump) {
    DumpCorpus(&rules, corpus, rule_idx);
    return 0;
  }

  if (OPTS.run_direct) {
    FuzzLoop(rule_idx, &rules, &cov, &mmap_state, corpus, nullptr);
    std::exit(0);
  }

  //may return 0 when not able to detect
  const auto nproc = std::thread::hardware_concurrency();

  resmack::fuzz::process_launchers::ForkLauncher launcher(
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
    }
  );

  size_t child_num;
  std::cout << "Creating " << OPTS.nprocs << " processes for fuzzing" << std::endl;
  for (child_num = 0; child_num < OPTS.nprocs; child_num++ ) {
    resmack::fuzz::Tracer* tracer = new resmack::fuzz::Tracer(
      &launcher,
      [&rules, rule_idx, &mmap_state](
        pid_t pid,
        resmack::fuzz::Tracer* tracer,
        resmack::fuzz::Tracee* tracee
      ) -> void {
        return HandleException(&rules, &mmap_state, rule_idx, pid, tracer, tracee);
      },
      [&rules, rule_idx, &mmap_state](
        pid_t pid,
        resmack::fuzz::Tracer* tracer,
        resmack::fuzz::Tracee* tracee
      ) -> void {
        return HandleTimeout(&rules, &mmap_state, rule_idx, pid, tracer, tracee);
      },
      child_num
    );
    tracer->Start();
    TRACERS.push_back(tracer);
  }

  STATUS_ARGS.state = &mmap_state;
  STATUS_ARGS.show_stats = OPTS.show_stats;
  STATUS_ARGS.feedback = &cov;
  pthread_create(&STATUS_THREAD, nullptr, DoLoopPrintStatus, (void*)&STATUS_ARGS);

  signal(SIGINT, sigint_handler);

    while (true) {
      if (
          (OPTS.max_crashes > 0 && mmap_state.GetNumCrashes() >= OPTS.max_crashes) ||
          (OPTS.max_iters > 0 && mmap_state.GetNumIterations() >= OPTS.max_iters) ||
          SHUTTING_DOWN.load()
      ) {
          DEBUG_PRINT("MET/EXCEED MAX CRASHES or NUM_ITERS\n");
          SHUTTING_DOWN.store(true);
          for (auto* tracer : TRACERS) {
              tracer->Stop(true);
          }
          break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  for (resmack::fuzz::Tracer* tracer: TRACERS) {
    tracer->Join();
    delete tracer;
  }

  DEBUG_PRINT("KILLING THINGS NOW\n");

  SHUTTING_DOWN.store(true);

  pthread_cancel(STATUS_THREAD);
  pthread_join(STATUS_THREAD, nullptr);
}
