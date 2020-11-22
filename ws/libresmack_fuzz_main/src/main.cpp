#include <ctime>
#include <string>
#include <set>

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

extern "C" int __lsan_is_turned_off() { return 1; }

__attribute__((visibility("default"))) int main(int argc, char** argv) {
  resmack::fuzz::ExternalFunctions EF;

  resmack::fuzz::states::MmapState state("/tmp/resmack.state");

  resmack::Rules rules = new resmack::Rules();
  size_t rule_idx = EF.ResmackGrammarInit(&rules);

  resmack::fuzz::Coverage cov;
  resmack::fuzz::NoopCoverage noop_cov;

  bool main_proc = (fork() != 0);

  //resmack::fuzz::Corpus corpus;
  resmack::Rand meta_rand;
  resmack::Rand build_rand(meta_rand.Next());
  build_rand.SetShouldRecord(true);

  std::string output;
  output.reserve(0x1000);

  resmack::fuzz::DirectTarget target;
  resmack::fuzz::TargetSettings settings;
  resmack::fuzz::TargetStats stats;
  resmack::BuildContext ctx(&output, &build_rand, 10);

  std::set<size_t> seen_covs;
  resmack::Vector<resmack::Vector<resmack::RandSnapshot>> corpus;

  resmack::Vector<resmack::RandSnapshot> mutated_replay;
  float start = clock() / (float)CLOCKS_PER_SEC;

  size_t counts = 0;
  while (true) {
    size_t corpus_len = corpus.size();
    if (corpus_len > 0 && meta_rand.Maybe()) {
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
      resmack::fuzz::MutateRandSnapshot(&meta_rand, replay, &mutated_replay);
      ctx.SetReplay(&mutated_replay);
    } else {
      ctx.SetReplay(NULL);
    }

    output.clear();
    build_rand.SnapshotClear();
    rules.Build(rule_idx, &ctx);
    state.IncNumIterations();
    counts++;

    if (main_proc && (counts % 0x100000) == 0) {
      float curr = clock() / (float)CLOCKS_PER_SEC;
      uint64_t num_iters = state.GetNumIterations();
      printf(
        "Iters: %lu | %0.2f iters/s\n",
        num_iters,
        (float)num_iters / (curr - start)
      );
    }

    stats.Tick();
    target.Launch(&cov, &output, &settings, &stats);
    //target.Launch(&noop_cov, &output, &settings, &stats);
    size_t cov_key = cov.GetStats().key;

    if (!seen_covs.contains(cov_key)) {
      std::cout << "New coverage with: " << output << ", key: " << cov_key << ", num: " << cov.GetStats().num << ", iters: " << counts << std::endl;
      corpus.emplace_back(*build_rand.GetSnapshots());
      seen_covs.emplace(cov_key);
    }

    if (stats.crashed) {
      std::cout << "CRASH! with " << output << " and " << counts << " iters" << std::endl;
      break;
    }
  }
}
