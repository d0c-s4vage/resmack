#include <vector>

#include "gtest/gtest.h"

#include "resmack/rand.hpp"
#include "resmack/rules.hpp"
#include "resmack/items/pre.hpp"
#include "resmack/items/post.hpp"
#include "resmack/items/raw.hpp"
#include "resmack/items/and.hpp"
#include "resmack/items/ref.hpp"
#include "resmack/items/flush.hpp"

#include "test_utils.hpp"

namespace resmack {
namespace items {

  TEST(Flush, Works)
  {
    Rules rules;
    rules.AddRule("pre-post", AND(PRE(V("(")), V("!"), POST(V(")"))))

      ->AddRule("flushed", FLUSHED(REF("pre-post")))
      ->AddRule("wrap-flushed", AND(
        PRE(V("[")), V("<?"), REF("flushed"), V("?>"), POST(V("]"))
      ))

      ->AddRule("not-flushed", REF("pre-post"))
      ->AddRule("wrap-not-flushed", AND(
        PRE(V("[")), V("<?"), REF("not-flushed"), V("?>"), POST(V("]"))
      ));
    
    Rand rand(100);
    std::string output;

    rules.Build("wrap-flushed", &output, &rand);
    EXPECT_EQ(output, "[<?(!)?>]");

    output.clear();
    rules.Build("wrap-not-flushed", &output, &rand);
    EXPECT_EQ(output, "[(<?!?>)]");
  }

}
}
