#include <vector>

#include "gtest/gtest.h"

#include "resmack/rand.hpp"
#include "resmack/rules.hpp"
#include "resmack/rule_man.hpp"
#include "resmack/item.hpp"
#include "resmack/items/and.hpp"
#include "resmack/items/id.hpp"
#include "resmack/items/raw.hpp"
#include "resmack/items/ref.hpp"

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
