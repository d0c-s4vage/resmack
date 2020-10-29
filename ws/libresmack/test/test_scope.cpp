#include <vector>

#include "gtest/gtest.h"

#include "resmack/rand.hpp"
#include "resmack/rules.hpp"
#include "resmack/rule_man.hpp"
#include "resmack/item.hpp"
#include "resmack/items/id.hpp"
#include "resmack/items/raw.hpp"
#include "resmack/items/scope.hpp"
#include "resmack/items/and.hpp"
#include "resmack/items/id.hpp"

#include "test_utils.hpp"

namespace resmack {
namespace items {

  TEST(Scope, Works)
  {
    /*
    rules.AddRule("dbContext", AND(
        PRE(AND(REF("db"), V(".Context(function(e) { var "), ID("dbContextId"), " = e.target;")),
        REF("dbContextId"),
        POST(V("})")),
    ));
    // when is the POST stream finalized? With each new rule that is built?
    rules.AddRule("dbContext.Test", AND(REF("dbContext"), V(".Test()")));

    (new Database()).Context(function(e) { var adsfkj = e.target; adsfkj.Test(); });

    post_output is prepended, not appended like the pre_output, potentially very
    slow
    */

    Rules rules;
    rules.AddRule("decl", ID("varname"))
      ->AddRule("scoped_decl", SCOPE(REF("decl")))
      ->AddRule("test", AND(REF("decl"), V(" "), REF("scoped_decl")));

    Rand rand(100);
    std::string output;

    rules.Build("test", &output, &rand);
    EXPECT_EQ(rules.GetRuleMan()->NumRules(), 4u);
    // varname should only have one option because we evaluate ID in the root
    // scope *once*
    EXPECT_EQ(rules.GetRuleMan()->GetRule("varname")->NumItems(), 1u);

    std::vector<std::string> parts;
    test_utils::SplitStr(output, " ", &parts);

    EXPECT_NE(parts[0], parts[1]);
  }

}
}
