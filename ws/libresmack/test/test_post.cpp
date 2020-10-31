#include <vector>

#include "gtest/gtest.h"

#include "resmack/rand.hpp"
#include "resmack/rules.hpp"
#include "resmack/items/and.hpp"
#include "resmack/items/id.hpp"
#include "resmack/items/post.hpp"
#include "resmack/items/pre.hpp"
#include "resmack/items/raw.hpp"
#include "resmack/items/ref.hpp"

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
      ->AddRule("wrapped", AND(REF("decorated"), V("WRAPPED")));

    Rand rand(100);
    std::string output;
    rules.Build("wrapped", &output, &rand);
    EXPECT_EQ(output, "--> WRAPPED <--");
  }

}
}
