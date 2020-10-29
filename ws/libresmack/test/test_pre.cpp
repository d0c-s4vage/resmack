#include <vector>

#include "gtest/gtest.h"

#include "resmack/rand.hpp"
#include "resmack/rules.hpp"
#include "resmack/items/pre.hpp"
#include "resmack/items/raw.hpp"
#include "resmack/items/and.hpp"
#include "resmack/items/id.hpp"

#include "test_utils.hpp"

namespace resmack {
namespace items {

  TEST(Pre, Works)
  {
    Rules rules;
    rules.AddRule("pre", AND(PRE(AND("hello")), V(" world")));

    Rand rand(100);
    std::string output;
    rules.Build("pre", &output, &rand);

    EXPECT_EQ(output, "hello world");
  }


  TEST(Pre, WithId)
  {
    Rules rules;
    rules.AddRule("pre", AND(
      PRE(AND(V("var "), ID("varname"), V(" =10; "))),
      REF("varname"),
      V("++")
    ));

    Rand rand(100);
    std::string output;
    rules.Build("pre", &output, &rand);

    std::vector<std::string> parts;
    test_utils::SplitStr(output, " ", &parts);

    // 0   1     2    3       
    // var XXXXX =10; XXXXX++
    EXPECT_EQ(parts[0], "var");
    EXPECT_EQ(parts[1] + "++", parts[3]);
  }

}
}
