#ifndef RESMACK_TEST_UTILS
#define RESMACK_TEST_UTILS

#include <map>
#include <string>

#include "rand.hpp"
#include "item.hpp"
#include "rules.hpp"
#include "build_context.hpp"

namespace test_utils {
  static std::string BuildItem(resmack::Item* item) {
    resmack::Rand rand;
    resmack::Rules rules;
    rules.AddRule("test", item);

    std::string output;
    rules.Build("test", &output, &rand);

    return output;
  }

  static void CountBuilds(int iters, resmack::Item* item, std::map<std::string, int>* counts) {
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
