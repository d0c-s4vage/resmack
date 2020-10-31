#include "gtest/gtest.h"

#include "resmack/rand.hpp"
#include "resmack/rules.hpp"
#include "resmack/item.hpp"
#include "resmack/items/ref.hpp"
#include "resmack/items/raw.hpp"

namespace resmack {
namespace items {

  TEST(Ref, Basic) {
    Rules rules;
    rules.AddRule("refd", new items::Raw("hello"));
    rules.AddRule("test", new items::Ref("refd"));

    std::string output;
    Rand rand;
    rules.Build("test", &output, &rand);

    EXPECT_EQ(output, "hello");
  }

  TEST(Ref, PruneUnresolvable) {
    Rules rules;
    rules.AddRule("test", new items::Ref("refd"));

    std::string output;
    Rand rand;
    rules.Finalize();

    EXPECT_EQ(rules.GetRuleMan()->ValidRule("test"), false);
  }

  TEST(Ref, PruneCircular) {
    Rules rules;
    rules.AddRule("rule1", new items::Ref("rule2"));
    rules.AddRule("rule2", new items::Ref("rule1"));

    std::string output;
    Rand rand;
    rules.Finalize();

    EXPECT_EQ(rules.GetRuleMan()->ValidRule("rule1"), false);
    EXPECT_EQ(rules.GetRuleMan()->ValidRule("rule2"), false);
  }

}
}
