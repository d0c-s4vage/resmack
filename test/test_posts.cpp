#include <vector>

#include "gtest/gtest.h"

#include "rand.hpp"
#include "rules.hpp"
#include "items/post.hpp"
#include "items/pre.hpp"
#include "items/raw.hpp"
#include "items/and.hpp"
#include "items/id.hpp"
#include "test_utils.hpp"

namespace resmack {
namespace items {

  TEST(Post, Works)
  {
    Rules rules;
    rules.AddRule("post", AND(V("hello "), POST(V("world"))));

    Rand rand(100);
    std::string output;
    rules.Build("post", &output, &rand);

    EXPECT_EQ(output, "hello world");
  }


  TEST(Post, WithId)
  {
    Rules rules;
    rules.AddRule("decorated", AND(PRE(V("--> ")), POST(V(" <--"))))
      ->AddRule("wrapped", AND(V("WRAPPED"), REF("decorated")))

      ->AddRule("dbContext", AND(
        PRE(V("(new Database()).open(function(e) { ")),
          V("e.target"),
        POST(V("})"))))
      ->AddRule("dbInner", AND(
        PRE(AND(REF("dbContext"), V(".operation(function(f) { "))),
          V("f.target"),
        POST(V("})"))))
      ->AddRule("dbContext.Test()", AND(REF("dbInner"), V(".Test()")))

      ->AddRule("l1", AND(PRE(V("l1[")), V("VAL"), POST(V("]"))))
      ->AddRule("l2", AND(PRE(REF("l1")), V(".l2"), POST(V(">"))))
      ->AddRule("l3", AND(PRE(REF("l2")), V(".l3"), POST(V("*"))))
      ->AddRule("l4", AND(REF("l3"), V(".test()")));

    Rand rand(100);
    std::string output;
    //rules.Build("wrapped", &output, &rand);

    //EXPECT_EQ(output, "--> WRAPPED <--");

    output.clear();
    // l1[VAL]
    rules.Build("dbContext.Test()", &output, &rand);
    std::cout << output << std::endl;
  }

}
}
