#include <vector>

#include "gtest/gtest.h"

#include "rand.hpp"
#include "rules.hpp"
#include "rule_man.hpp"
#include "item.hpp"
#include "items/id.hpp"
#include "items/raw.hpp"
#include "items/and.hpp"
#include "test_utils.hpp"

namespace resmack {
namespace items {

  TEST(Id, Works)
  {
    Rules rules;
    rules.AddRule("test", AND(ID("new_rule"), V(" = "), REF("new_rule")));

    Rand rand(100);
    std::string output;

    rules.Build("test", &output, &rand);
    EXPECT_EQ(rules.GetRuleMan()->NumRules(), 2u);

    std::vector<std::string> parts;
    test_utils::SplitStr(output, " ", &parts);

    EXPECT_EQ(parts[0], parts[2]);
    EXPECT_EQ(parts[1], "=");
  }

}
}
