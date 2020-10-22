#include "gtest/gtest.h"

#include "types.hpp"
#include "calc/reach.hpp"
#include "rules.hpp"
#include "item.hpp"
#include "items/or.hpp"
#include "items/raw.hpp"
#include "items/ref.hpp"

#include "test_utils.hpp"

namespace resmack {
namespace items {

  TEST(Or, OneOption)
  {
    Or *or_ = (new Or())
      ->AddItem(new Raw("hello"))
      ->AddItem(new Raw("world"));

    EXPECT_EQ(or_->NumItems(), 2u);

    Map<std::string, size_t> counts;
    test_utils::CountBuilds(100, or_, &counts);

    EXPECT_EQ(counts.contains("hello"), true);
    EXPECT_GT(counts["hello"], 0u);

    EXPECT_EQ(counts.contains("world"), true);
    EXPECT_GT(counts["world"], 0u);
  }

  TEST(Or, VariableOptions)
  {
    Or *or_ = new Or();
    or_->AddItems(
        new Raw("hello"),
        new Raw("world"),
        new Raw("test"),
        NULL);

    EXPECT_EQ(or_->NumItems(), 3u);

    Map<std::string, size_t> counts;
    test_utils::CountBuilds(100, or_, &counts);

    EXPECT_EQ(counts.contains("hello"), true);
    EXPECT_GT(counts["hello"], 0u);

    EXPECT_EQ(counts.contains("world"), true);
    EXPECT_GT(counts["world"], 0u);

    EXPECT_EQ(counts.contains("test"), true);
    EXPECT_GT(counts["test"], 0u);
  }

  TEST(Or, SetsChoiceIndices)
  {
    Or *or_ = new Or();
    or_->AddItems(
        new Raw("hello"),
        new Raw("world"),
        new Raw("test"),
        NULL);

    RuleManager rule_man;
    calc::Reach reach(&rule_man);
    calc::RefDepth ref_depth(&rule_man);
    reach.CalcItem(or_);
    ref_depth.CalcItem(or_);

    EXPECT_EQ(or_->NumChoicesItems(), 3u);
    EXPECT_EQ(or_->NumShortestItems(), 3u);
  }

}
}
