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

__attribute__((visibility("default"))) int main(int argc, char** argv) {
  resmack::fuzz::ExternalFunctions EF;

  resmack::Rules rules = new resmack::Rules();
  rules.AddRule("fruit", OR("apples", "bananas", "grapes", "pears", "peaches"))
    ->AddRule("fruit-list", AND_S(" ",
      REF("fruit"),
      OPT(AND_S(" ", OR("or", "and", "with", "without"), REF("fruit-list")))
    ))
    ->AddRule("verb", OR("eat", "throw", "stomp on"))
    ->AddRule("subject", OR("I", "we", "you"))
    ->AddRule("sentence", AND_S(" ", REF("subject"), REF("verb"), REF("fruit-list")));//, REF("verb"), REF("fruit-list")));
  size_t rule_idx;
  if (!rules.GetRuleMan()->IndexOf("sentence", &rule_idx)) {
    std::cout << "Invalid rules" << std::endl;
    return 1;
  }

  resmack::fuzz::Coverage cov;
  resmack::fuzz::NoopCoverage noop_cov;

  //resmack::fuzz::Corpus corpus;
  resmack::Rand meta_rand;
  resmack::Rand build_rand(meta_rand.Next());
  build_rand.SetShouldRecord(true);

  std::string output;
  output.reserve(0x1000);

  resmack::fuzz::DirectTarget target;
  resmack::fuzz::TargetSettings settings;
  resmack::fuzz::TargetStats stats;
  resmack::BuildContext ctx(&output, &build_rand, 100);

  std::set<size_t> seen_covs;
  resmack::Vector<resmack::Vector<resmack::RandSnapshot>> corpus;

  resmack::Vector<resmack::RandSnapshot> mutated_replay;

  size_t counts = 0;
  for (int i = 0; i < 100; i++) {
    seen_covs.clear();
    corpus.clear();

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
      counts++;

      stats.Tick();
      target.Launch(&cov, &output, &settings, &stats);
      //target.Launch(&noop_cov, &output, &settings, &stats);
      size_t cov_key = cov.GetStats().key;

      //std::cout << output << std::endl;
      if (!seen_covs.contains(cov_key)) {
        //std::cout << "New coverage with: " << output << ", key: " << cov_key << std::endl;
        corpus.emplace_back(*build_rand.GetSnapshots());
        seen_covs.emplace(cov_key);
      }

      if (stats.crashed) {
        std::cerr << ".";
        break;
      }
    }
  }
  std::cout << std::endl << counts <<std::endl;
}
