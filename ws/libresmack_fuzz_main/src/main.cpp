#include <chrono>
#include <cstddef>
#include <string>
#include <set>
#include <ctime>
#include <ratio>

#include "resmack/build_context.hpp"
#include "resmack/fuzz/feedbacks/noop.hpp"
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

extern "C" int __lsan_is_turned_off() { return 1; }

void LoopPrintStatus(resmack::fuzz::states::MmapState* state, bool show_stats) {
  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
  std::chrono::high_resolution_clock::time_point end;
  uint64_t start_iters = state->GetNumIterations();
  int sleep_amt = 1;
  resmack::fuzz::Corpus* corpus = state->GetCorpus();
  resmack::fuzz::StateStats* stats = state->GetStats();
  while (true) {
    sleep(sleep_amt++);
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

__attribute__((visibility("default"))) int main(int argc, char** argv) {
  resmack::fuzz::ExternalFunctions EF;

  resmack::fuzz::states::MmapState state("/tmp/resmack.state");
  resmack::fuzz::Corpus* corpus = state.GetCorpus();

  resmack::Rules rules = new resmack::Rules();
  size_t rule_idx = EF.ResmackGrammarInit(&rules);

  resmack::fuzz::Coverage cov;
  resmack::fuzz::NoopCoverage noop_cov;

  bool is_main_proc = true;

  int child_num;
  for (child_num = 0; child_num < 2; child_num++ ) {
    if (!fork()) {
      is_main_proc = false;
      break;
    }
  }

  if (is_main_proc) {
    LoopPrintStatus(&state, true);
  }

  resmack::Rand meta_rand;
  resmack::Rand build_rand(meta_rand.Next());
  build_rand.SetShouldRecord(true);

  std::string output;

  resmack::fuzz::DirectTarget target;
  resmack::fuzz::TargetSettings settings;
  resmack::fuzz::TargetStats stats(0x1000);
  resmack::BuildContext ctx(&output, &build_rand, 10);

  std::set<size_t> seen_covs;

  resmack::Vector<resmack::RandSnapshot> mutated_replay;

  size_t counts = 0;
  while (true) {
    if (corpus->NumItems() > 0 && meta_rand.Maybe()) {
      resmack::Vector<resmack::RandSnapshot>* replay;
      RECORD_STAT(&stats, resmack::fuzz::SampleTypes::CORPUS, {
        replay = corpus->GetItem(&meta_rand);
      });
      RECORD_STAT(&stats, resmack::fuzz::SampleTypes::MUTATE, {
        resmack::fuzz::MutateRandSnapshot(&meta_rand, replay, &mutated_replay);
      });
      ctx.SetReplay(&mutated_replay);
    } else {
      ctx.SetReplay(NULL);
    }

    output.clear();
    build_rand.SnapshotClear();

    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::GENERATE, {
      rules.Build(rule_idx, &ctx);
    });
    counts++;

    if ((counts % 0x1000) == 0) {
      state.IncNumIterations(0x1000);
      state.SyncStats(&stats);
    }

    stats.Tick();
    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::TARGET, {
      target.Launch(&cov, &output, &settings, &stats);
    });
    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::TARGET_RESET, {
      target.Reset();
    });
    size_t cov_key = cov.GetStats().key;

    RECORD_STAT(&stats, resmack::fuzz::SampleTypes::CORPUS, {
      if (corpus->AddRandSnapshotIfNotSeen(build_rand.GetSnapshots(), cov_key)) {
        std::cout << "New coverage with: " << output << ", key: " << cov_key << ", num: " << cov.GetStats().num << ", iters: " << counts << std::endl;
      }
    });

    if (stats.crashed) {
      state.IncNumCrashes();
      std::cout << "CRASH! with " << output << " and " << state.GetNumIterations() << " iters" << std::endl;
    }
  }
}
