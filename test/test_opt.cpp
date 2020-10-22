#include <string>

#include "gtest/gtest.h"

#include "types.hpp"
#include "items/opt.hpp"
#include "items/raw.hpp"

#include "test_utils.hpp"

namespace resmack {
namespace items {

  TEST(Opt, GeneratesMaybe) {
    Map<std::string, size_t> counts;
    Opt* opt = new Opt(new Raw("test"));
    test_utils::CountBuilds(100, opt, &counts);

    EXPECT_EQ(counts.size(), 2u);
    EXPECT_GT(counts["test"], 0u);
    EXPECT_LT(counts["test"], 100u);
    EXPECT_GT(counts[""], 0u);
    EXPECT_LT(counts[""], 100u);
  }

}
}
