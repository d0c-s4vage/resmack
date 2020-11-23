#include <chrono>
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

void LoopPrintStatus(resmack::fuzz::states::MmapState* state) {
  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
  std::chrono::high_resolution_clock::time_point end;
  //float start = clock() / (float)CLOCKS_PER_SEC;
  int sleep_amt = 1;
  while (true) {
    sleep(sleep_amt++);
    end = std::chrono::high_resolution_clock::now();
    uint64_t num_iters = state->GetNumIterations();
    std::chrono::duration<double> span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    printf(
      "Iters: %lu | %0.2f iters/s | %0.2f seconds\n",
      num_iters,
      (float)num_iters / span.count(),
      span.count()
    );
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

  //bool main_proc = true;
  for (int i = 0; i < 2; i++ ) {
    if (!fork()) {
      is_main_proc = false;
      break;
    }
  }

  if (is_main_proc) {
    LoopPrintStatus(&state);
  }

  //resmack::fuzz::Corpus corpus;
  resmack::Rand meta_rand;
  resmack::Rand build_rand(meta_rand.Next());
  build_rand.SetShouldRecord(true);

  std::string output;

  resmack::fuzz::DirectTarget target;
  resmack::fuzz::TargetSettings settings;
  resmack::fuzz::TargetStats stats;
  resmack::BuildContext ctx(&output, &build_rand, 10);

  std::set<size_t> seen_covs;
  //resmack::Vector<resmack::Vector<resmack::RandSnapshot>> corpus;

  resmack::Vector<resmack::RandSnapshot> mutated_replay;

  size_t counts = 0;
  while (true) {
    //size_t corpus_len = corpus.size();
    if (corpus->NumItems() > 0 && meta_rand.Maybe()) {
      resmack::Vector<resmack::RandSnapshot>* replay = corpus->GetItem(&meta_rand);
    /*
    if (corpus->corpus_len > 0 && meta_rand.Maybe()) {
      size_t rand_idx;
      if (corpus_len > 4) {
        size_t half_size = corpus_len >> 1;
        size_t first_half = corpus_len - half_size;
        // double the odds of the last half
        size_t new_size = corpus_len + half_size;
        size_t tmp = meta_rand.Next() % new_size;
        if (tmp < first_half) {
          rand_idx = tmp;
        } else {
          rand_idx = first_half + (tmp - first_half) / 2;
        }
      } else {
        rand_idx = meta_rand.Next() % corpus_len;
      }

      resmack::Vector<resmack::RandSnapshot>* replay = &corpus[rand_idx];
      */
      resmack::fuzz::MutateRandSnapshot(&meta_rand, replay, &mutated_replay);
      ctx.SetReplay(&mutated_replay);
    } else {
      ctx.SetReplay(NULL);
    }

    output.clear();
    build_rand.SnapshotClear();
    rules.Build(rule_idx, &ctx);
    counts++;

    if ((counts % 0x10000) == 0) {
      state.IncNumIterations(0x10000);
    }

    stats.Tick();
    target.Launch(&cov, &output, &settings, &stats);
    size_t cov_key = cov.GetStats().key;

    if (!seen_covs.contains(cov_key)) {
      std::cout << "New coverage with: " << output << ", key: " << cov_key << ", num: " << cov.GetStats().num << ", iters: " << counts << std::endl;
      //corpus.emplace_back(*build_rand.GetSnapshots());
      corpus->AddRandSnapshot(build_rand.GetSnapshots(), 0);
      seen_covs.emplace(cov_key);
    }

    if (stats.crashed) {
      std::cout << "CRASH! with " << output << " and " << state.GetNumIterations() << " iters" << std::endl;
      break;
    }
  }
}
