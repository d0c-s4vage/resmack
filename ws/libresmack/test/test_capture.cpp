#include <vector>

#include "gtest/gtest.h"
#include "fmt/compile.h"

#include "resmack/rand.hpp"
#include "resmack/rules.hpp"
#include "resmack/items/pre.hpp"
#include "resmack/items/capture.hpp"
#include "resmack/items/raw.hpp"
#include "resmack/items/and.hpp"
#include "resmack/items/ref.hpp"
#include "resmack/items/str.hpp"

#include "test_utils.hpp"

namespace resmack {
namespace items {

  TEST(Capture, Works)
  {
    Rules rules;
    rules
      .AddRule("capture1", AND(
        PRE(AND(V("var "), CAPTURE(STR(5)), V(" = new Object(); "))),
        CAPTURED
      ))
      ->AddRule("ref_captured", AND(REF("capture1"), V(".hello()")));
    
    Rand rand(100);
    std::string output;

    rules.Build("ref_captured", &output, &rand);

    std::vector<std::string> parts;
    test_utils::SplitStr(output, " ", &parts);

    EXPECT_EQ(parts.size(), 6u);

    std::string& name = parts[1];
    EXPECT_EQ(
      output,
      fmt::format("var {0} = new Object(); {0}.hello()", name)
    );
  }

  TEST(Capture, Nested)
  {
    Rules rules;
    rules
      .AddRule("capture1", AND(
        PRE(AND(V("var "), CAPTURE(STR(5)), V(" = new Object(); "))),
        CAPTURED
      ))
      ->AddRule("capture2", AND(
        PRE(AND(V("var "), CAPTURE(STR(5)), V(" = "), REF("capture1"), V(".func(); "))),
        CAPTURED
      ))
      ->AddRule("ref_captured", AND(REF("capture2"), V(".hello()")));
    
    Rand rand(100);
    std::string output;

    rules.Build("ref_captured", &output, &rand);

    std::vector<std::string> parts;
    test_utils::SplitStr(output, " ", &parts);

    EXPECT_EQ(parts.size(), 10u);

    // 0    1    2  3    4        5   6
    // var NAME1 = new Object(); var NAME2 = NAME1.func(); NAME2.hello()
    std::string& name1 = parts[1];
    std::string& name2 = parts[6];
    std::string empty;

    EXPECT_EQ(
      output,
      fmt::format("var {0} = new Object(); var {1} = {0}.func(); {1}.hello()",
        name1,
        name2
      )
    );
  }

}
}
