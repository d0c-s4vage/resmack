#include <string>

#include "gtest/gtest.h"

#include "resmack/types.hpp"
#include "resmack/rules.hpp"
#include "resmack/item.hpp"
#include "resmack/items/str.hpp"

#include "test_utils.hpp"

namespace resmack {
namespace items {

  TEST(Str, CorrectLength) {
    for (size_t i = 0; i < 100; i++) {
      Str* str = new Str(2, 3, "A");
      std::string built = test_utils::BuildItem(str);
      EXPECT_EQ(built, "AA");
    }

    Map<std::string, size_t> counts;
    // 2, 3, 4 are the possible lengths
    Str* str = new Str(2, 5, "A");
    test_utils::CountBuilds(100, str, &counts);

    EXPECT_GT(counts["AA"], 0u);
    EXPECT_GT(counts["AAA"], 0u);
    EXPECT_GT(counts["AAA"], 0u);
  }

  TEST(Str, FullCharset) {
    Map<std::string, size_t> counts;
    // 2, 3, 4 are the possible lengths
    Str* str = new Str(1, 2, "ABCD");
    test_utils::CountBuilds(1000, str, &counts);

    EXPECT_GT(counts["A"], 0u);
    EXPECT_GT(counts["B"], 0u);
    EXPECT_GT(counts["C"], 0u);
    EXPECT_GT(counts["D"], 0u);
  }

}
}
