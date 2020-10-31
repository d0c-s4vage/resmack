#include "gtest/gtest.h"

#include "resmack/rules.hpp"
#include "resmack/item.hpp"
#include "resmack/items/and.hpp"
#include "resmack/items/raw.hpp"
#include "resmack/items/ref.hpp"

#include "test_utils.hpp"

namespace resmack {
namespace items {

  TEST(And, WithSep)
  {
    And* and_ = (new And("-"))\
      ->AddItem(new Raw("hello"))\
      ->AddItem(new Raw("world"));

    std::string built = test_utils::BuildItem(and_);
    EXPECT_EQ(built, "hello-world");
  }

  TEST(And, NoSep)
  {
    And* and_ = (new And())\
      ->AddItem(new Raw("hello"))\
      ->AddItem(new Raw("world"));

    std::string built = test_utils::BuildItem(and_);
    EXPECT_EQ(built, "helloworld");
  }

  TEST(And, NoItems)
  {
    And* and_ = new And();

    std::string built = test_utils::BuildItem(and_);
    EXPECT_EQ(built, "");
  }

  TEST(And, ComplexItem)
  {
    std::string output;
    Rand rand;
    Rules rules;

    rules.AddRule("other", new Raw("hello"));
    rules.AddRule("test", (new And("-"))\
        ->AddItem(new Ref("other"))\
        ->AddItem(new Raw("world")));
    rules.Build("test", &output, &rand);

    EXPECT_EQ(output, "hello-world");
  }
}
}
