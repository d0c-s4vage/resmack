#ifndef RESMACK_TEST_UTILS
#define RESMACK_TEST_UTILS

#include <string>

#include "resmack/types.hpp"
#include "resmack/rand.hpp"
#include "resmack/item.hpp"
#include "resmack/rules.hpp"
#include "resmack/build_context.hpp"

namespace test_utils {
  __attribute__((unused))
  static std::string BuildItem(resmack::Item* item) {
    resmack::Rand rand;
    resmack::Rules rules;
    rules.AddRule("test", item);

    std::string output;
    rules.Build("test", &output, &rand);

    return output;
  }

  __attribute__((unused))
  static void CountBuilds(int iters, resmack::Item* item, resmack::Map<std::string, size_t>* counts) {
    resmack::Rand rand;
    resmack::Rules rules;
    rules.AddRule("test", item);

    std::string output;

    for (int i = 0; i < iters; i++) {
      output.clear();
      rules.Build("test", &output, &rand);
      (*counts)[output]++;
    }
  }

  __attribute__((unused))
  static void SplitStr(const std::string& input, const std::string& split, std::vector<std::string>* output) {
    size_t last_idx = 0;
    size_t split_idx = input.find(split, last_idx);
    while (split_idx != std::string::npos) {
      output->emplace_back(input.substr(last_idx, split_idx - last_idx));
      last_idx = split_idx + split.size();
      split_idx = input.find(split, last_idx);
    }
    output->emplace_back(input.substr(last_idx, input.size() - last_idx));
  }
}

#endif
