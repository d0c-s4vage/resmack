#include "gtest/gtest.h"

#include "rules.hpp"
#include "item.hpp"
#include "items/and.hpp"
#include "items/raw.hpp"
#include "items/ref.hpp"

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
