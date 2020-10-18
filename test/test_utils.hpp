#ifndef RESMACK_TEST_UTILS
#define RESMACK_TEST_UTILS

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
}

#endif
