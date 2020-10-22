#include <string>

#include "gtest/gtest.h"

#include "types.hpp"
#include "items/int.hpp"

#include "test_utils.hpp"

namespace resmack {
namespace items {

  TEST(Int, CorrectLength) {
    Map<std::string, size_t> counts;
    Int* i = new Int(2, 3);
    test_utils::CountBuilds(100, i, &counts);
    EXPECT_EQ(counts.size(), 1u);
    EXPECT_GT(counts["2"], 0u);

    counts.clear();
    // 2, 3, 4 are the possible values
    Int* i2 = new Int(2, 5);
    test_utils::CountBuilds(100, i2, &counts);

    EXPECT_GT(counts["2"], 0u);
    EXPECT_GT(counts["3"], 0u);
    EXPECT_GT(counts["4"], 0u);
  }

}
}
