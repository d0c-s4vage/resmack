#ifndef RESMACK_TEST_UTILS
#define RESMACK_TEST_UTILS

#include <string>

#include "types.hpp"
#include "rand.hpp"
#include "item.hpp"
#include "rules.hpp"
#include "build_context.hpp"

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
}

#endif
