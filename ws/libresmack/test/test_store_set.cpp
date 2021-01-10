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
#include "resmack/items/store.hpp"
#include "resmack/items/str.hpp"
#include "resmack/items/scope.hpp"
#include "resmack/items/capture.hpp"
#include "resmack/items/pre.hpp"
#include "resmack/items/post.hpp"
#include "resmack/items/opt.hpp"

#include "test_utils.hpp"

namespace resmack {
namespace items {

  TEST(Store, Works)
  {
    Rules rules;
    rules.AddRule("set_val", AND(STORE("dyn_rule", V("DYN_VAL")), V(" hello")))
      ->AddRule("use_val", AND(REF("set_val"), V(" "), REF("dyn_rule"), V(" goodbye")));

    Rand rand(100);
    std::string output;

    rules.Build("use_val", &output, &rand);

    EXPECT_EQ(rules.GetRuleMan()->NumRules(), 3u);
    EXPECT_EQ(rules.GetRuleMan()->GetRule("dyn_rule")->NumItems(), 1u);
    EXPECT_EQ("DYN_VAL hello DYN_VAL goodbye", output);
  }

  TEST(Set, Works)
  {
    Rules rules;
    rules.AddRule("set_val", AND(SET("dyn_rule", V("DYN_VAL")), V(" hello")))
      ->AddRule("use_val", AND(REF("set_val"), V(" "), REF("dyn_rule"), V(" goodbye")));

    Rand rand(100);
    std::string output;

    rules.Build("use_val", &output, &rand);

    EXPECT_EQ(rules.GetRuleMan()->NumRules(), 3u);
    EXPECT_EQ(rules.GetRuleMan()->GetRule("dyn_rule")->NumItems(), 1u);
    EXPECT_EQ("DYN_VAL hello DYN_VAL goodbye", output);
  }

  TEST(Set, PythonIndentation) {
    Rules rules;
    rules.AddRule("indentation", V(""));
    rules.
      AddRule("statements", AND(
        REF("indented-statement"), OPT(AND(V("\n"), REF("statements")))
      ))
      ->AddRule("indented-statement", AND(
        SCOPED_REF("indentation"), REF("statement")
      ))
      ->AddRule("statement", V("print('hello world')"))
      ->AddRule("statement", AND(
        V("if True:\n"),
        ISET("prev-indentation", AND(SCOPED_REF("indentation"))),
        ISET("indentation", AND(SCOPED_REF("indentation"), V("  "))),
        REF("statements"),
        ISET("indentation", AND(SCOPED_REF("prev-indentation")))
      ))
      ->AddRule("function", AND(
        PRE(AND(
          V("def "), CAPTURE(V("the_function")), V("():\n"),
          SCOPE_PUSH,
          ISET("indentation", AND(SCOPED_REF("indentation"), V("  "))),
          REF("statements"),
          V("\n")
        )),
        CAPTURED,
        POST(SCOPE_POP)
      ))
      ->AddRule("function-call", AND(REF("indentation"), REF("function"), V("()")));

    Rand rand(101);
    std::string output;
    
    rules.Build("function-call", &output, &rand);

    EXPECT_EQ(
      output,
      "def the_function():\n"
      "  if True:\n"
      "    print('hello world')\n"
      "  if True:\n"
      "    print('hello world')\n"
      "    if True:\n"
      "      print('hello world')\n"
      "    print('hello world')\n"
      "    print('hello world')\n"
      "the_function()"
    );
  }

}
}
