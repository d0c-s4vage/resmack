#include "gtest/gtest.h"

#include "rand.hpp"
#include "rules.hpp"
#include "item.hpp"
#include "items/ref.hpp"
#include "items/raw.hpp"

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

    EXPECT_EQ(rules.GetRules()->contains("test"), false);
  }

  TEST(Ref, PruneCircular) {
    Rules rules;
    rules.AddRule("rule1", new items::Ref("rule2"));
    rules.AddRule("rule2", new items::Ref("rule1"));

    std::string output;
    Rand rand;
    rules.Finalize();

    EXPECT_EQ(rules.GetRules()->contains("rule1"), false);
    EXPECT_EQ(rules.GetRules()->contains("rule2"), false);
  }

}
}
