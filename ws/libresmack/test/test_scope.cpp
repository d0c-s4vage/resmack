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
#include "resmack/items/scope.hpp"
#include "resmack/items/pre.hpp"
#include "resmack/items/post.hpp"
#include "resmack/items/capture.hpp"

#include "test_utils.hpp"

namespace resmack {
namespace items {

  TEST(Scope, Works)
  {
    Rules rules;
    rules.AddRule("decl", ID("varname"))
      ->AddRule("scoped_decl", SCOPE(REF("decl")))
      ->AddRule("test", AND(REF("decl"), V(" "), REF("scoped_decl")));
    rules.Finalize();

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

  TEST(Scope, WithPrePostCapture) {
    Rand rand;
    Rules rules;
    std::string output;

    rules
      .AddRule("CacheStorage", V("window.caches"))

      ->AddRule("Cache", AND(
        PRE(AND(
          REF("CacheStorage"), V(".open('"), ID("CacheName"), V("').then(function("), SCOPE_PUSH, CAPTURE(ID("Cache")), V(") {")
        )),
        CAPTURED,
        POST(AND(V("})"), SCOPE_POP))
      ))
      ->AddRule("Cache.keys", AND(
        PRE(AND(
          REF("Cache"), V(".keys().then(function("), SCOPE_PUSH, CAPTURE(ID("Cache.keys")), V(") {")
        )),
        CAPTURED,
        POST(AND(V("})"), SCOPE_POP))
      ))
      ->AddRule("Cache.delete", AND(REF("Cache"), V(".delete("), REF("Cache.keys"), V("[0])")));

    EXPECT_EQ(rules.GetRuleMan()->GetRule("Cache")->NumItems(), 1u);
    rules.Build("Cache.delete", &output, &rand);
    EXPECT_EQ(rules.GetRuleMan()->GetRule("Cache")->NumItems(), 1u);
  }

}
}
