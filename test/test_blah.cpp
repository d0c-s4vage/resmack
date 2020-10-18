#include "gtest/gtest.h"

#include "build_context.hpp"
#include "item.hpp"
#include "items/and.hpp"
#include "items/raw.hpp"

#include "test_utils.hpp"

namespace resmack {
namespace items {

  TEST(ResmackAnd, andWithSep)
  {
    And* and_ = (new And("-"))\
      ->AddItem(new Raw("hello"))\
      ->AddItem(new Raw("world"));

    std::string built = test_utils::BuildItem(and_);
    EXPECT_EQ(built, "hello-world");
  }

}
}
