#include "gtest/gtest.h"

#include "resmack/build_context.hpp"
#include "resmack/rand.hpp"
#include "resmack/rules.hpp"
#include "resmack/item.hpp"
#include "resmack/items/or.hpp"

#include "calc/reach.hpp"
#include "calc/ref_depth.hpp"

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
        nullptr);

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
    Or* or_ = new Or();
    or_->AddItems(
        new Raw("hello"),
        new Raw("world"),
        new Raw("test"),
        nullptr);

    RuleManager rule_man;
    rule_man.Init();
    calc::Reach reach(&rule_man);
    calc::RefDepth ref_depth(&rule_man);
    reach.CalcItem(or_);
    ref_depth.CalcItem(or_);

    EXPECT_EQ(or_->NumChoicesItems(), 3u);
    EXPECT_EQ(or_->NumShortestItems(), 3u);

    delete or_;
  }

  TEST(Or, SetsShortestIndices)
  {
    Rules rules;
    rules.AddRule("depth0", new Raw("depth0"));
    rules.AddRule("depth1", new Ref("depth0"));
    rules.AddRule("depth2", new Ref("depth1"));
    rules.AddRule("or", new Raw("Hello"));
    rules.AddRule("or", new Raw("Hello Again"));
    rules.AddRule("or", new Ref("depth0"));
    rules.AddRule("or", new Ref("depth1"));
    rules.AddRule("or", new Ref("depth2"));
    
    rules.Finalize();
    Or* or_ = rules.GetRuleMan()->GetRule("or");

    EXPECT_EQ(or_->NumShortestItems(), 2u);
    EXPECT_EQ(or_->NumChoicesItems(), 5u);
  }


  TEST(Or, BuildsShortest)
  {
    Rules rules;
    rules.AddRule("depth1", new Raw("depth1"));

    rules.AddRule("depth2_2", new Raw("depth2"));
    rules.AddRule("depth2", new Ref("depth2_2"));
    rules.AddRule("depth2", new Raw("depth1"));

    rules.AddRule("depth3_3", new Raw("depth3"));
    rules.AddRule("depth3_2", new Ref("depth3_3"));
    rules.AddRule("depth3_2", new Raw("depth2"));
    rules.AddRule("depth3", new Ref("depth3_2"));
    rules.AddRule("depth3", new Raw("depth1"));

    rules.AddRule("or", new Raw("Hello"));
    rules.AddRule("or", new Raw("Hello Again"));
    rules.AddRule("or", new Ref("depth1"));
    rules.AddRule("or", new Ref("depth2"));
    rules.AddRule("or", new Ref("depth3"));
    
    rules.Finalize();

    Rand rand(100);
    Map<std::string, size_t> counts;
    std::string output;

    Vector<std::pair<size_t, Vector<std::string>>> expecteds = {
      { 0, { "Hello", "Hello Again" } },
      { 1, { "Hello", "Hello Again", "depth1" } },
      { 2, { "Hello", "Hello Again", "depth1", "depth2" } },
      { 3, { "Hello", "Hello Again", "depth1", "depth2", "depth3" } },
    };

    for (auto pair: expecteds) {
      output.clear();
      counts.clear();

      size_t max_depth = pair.first;
      Vector<std::string> expected_vals = pair.second;

      for (size_t i = 0; i < 100; i++ ) {
        output.clear();
        rules.Build("or", &output, &rand, max_depth);
        counts[output] += 1;
      }

      EXPECT_EQ(counts.size(), expected_vals.size());
      for (std::string val: expected_vals) {
        EXPECT_TRUE(counts.contains(val));
      }
    }
  }
}
}
