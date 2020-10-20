#include <map>

#include "gtest/gtest.h"

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

    EXPECT_EQ(or_->NumItems(), 2);

    std::map<std::string, int> counts;
    test_utils::CountBuilds(100, or_, &counts);

    EXPECT_EQ(counts.contains("hello"), true);
    EXPECT_GT(counts["hello"], 0);

    EXPECT_EQ(counts.contains("world"), true);
    EXPECT_GT(counts["world"], 0);
  }

  TEST(Or, VariableOptions)
  {
    Or *or_ = new Or();
    or_->AddItems(
        new Raw("hello"),
        new Raw("world"),
        new Raw("test"),
        NULL);

    EXPECT_EQ(or_->NumItems(), 3);

    std::map<std::string, int> counts;
    test_utils::CountBuilds(100, or_, &counts);

    EXPECT_EQ(counts.contains("hello"), true);
    EXPECT_GT(counts["hello"], 0);

    EXPECT_EQ(counts.contains("world"), true);
    EXPECT_GT(counts["world"], 0);

    EXPECT_EQ(counts.contains("test"), true);
    EXPECT_GT(counts["test"], 0);
  }

  TEST(Or, SetsChoiceIndices)
  {
    Or *or_ = new Or();
    or_->AddItems(
        new Raw("hello"),
        new Raw("world"),
        new Raw("test"),
        NULL);

    std::map<std::string, Or*> map;
    calc::Reach reach(&map);
    calc::RefDepth ref_depth(&map);
    reach.CalcItem(or_);
    ref_depth.CalcItem(or_);

    EXPECT_EQ(or_->NumChoicesItems(), 3);
    EXPECT_EQ(or_->NumShortestItems(), 3);
  }

}
}
