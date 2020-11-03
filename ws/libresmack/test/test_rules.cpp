#include "gtest/gtest.h"

#include "resmack/rand.hpp"
#include "resmack/rules.hpp"
#include "resmack/rule_man.hpp"
#include "resmack/item.hpp"
#include "resmack/types.hpp"
#include "resmack/items/and.hpp"
#include "resmack/items/ref.hpp"
#include "resmack/items/raw.hpp"
#include "resmack/items/pre.hpp"
#include "resmack/items/post.hpp"
#include "resmack/items/or.hpp"
#include "resmack/items/int.hpp"
#include "resmack/items/str.hpp"

#include "test_utils.hpp"

namespace resmack {
namespace items {

  TEST(Rules, ReplayRandState) {
    Rules rules;

    // Will create a rand snapshot with:
    //   [0] = rule: 'top', ref_depth: 0
    //   [1] = rule: 'ref1|ref2', ref_depth: 1
    //   [2] = rule: 'numbers|letters', ref_depth: 2
    rules.AddRule("letters", STR(2, 20))
      ->AddRule("numbers", INT(0, 0x10000))
      ->AddRule("ref1", AND_S("-",
        STR(3, 5, "abcd"),
        OR(REF("numbers"), REF("letters")),
        V("hello")
      ))
      ->AddRule("ref2", AND_S("-",
        STR(3, 5, "WXYZ"),
        OR(REF("numbers"), REF("letters")),
        V("goodbye")
      ))
      ->AddRule("top", OR(REF("ref1"), REF("ref2")));

    Rand rand(100);
    rand.SetShouldRecord(true);
    std::string output;
    BuildContext ctx(&output, &rand, 10);

    size_t rule_idx = 0;
    EXPECT_TRUE(rules.GetRuleMan()->IndexOf("top", &rule_idx));

    rules.Build(rule_idx, &ctx);

    std::vector<std::string> orig_splits;
    test_utils::SplitStr(output, "-", &orig_splits);

    Vector<RandSnapshot> replay;
    replay.emplace_back((*rand.GetSnapshots())[0]);
    replay.emplace_back((*rand.GetSnapshots())[1]);
    replay.emplace_back((*rand.GetSnapshots())[2]);

    // new rand state for generating the inner value, the 'numbers|letters'
    // snapshot
    rand.CopyState(replay[2].state);
    ctx.replay = &replay;

    rand.SnapshotClear();

    output.clear();
    rules.Build(rule_idx, &ctx);

    std::vector<std::string> new_splits;
    test_utils::SplitStr(output, "-", &new_splits);

    EXPECT_EQ(orig_splits[0], new_splits[0]);
    EXPECT_NE(orig_splits[1], new_splits[1]);
    EXPECT_EQ(orig_splits[2], new_splits[2]);

    Vector<RandSnapshot> replay2(*rand.GetSnapshots());
    // should be able to replay the exact same thing as when using the mutated
    // replay
    ctx.replay = &replay2;
    output.clear();
    ctx.Reset();
    rules.Build(rule_idx, &ctx);

    std::vector<std::string> new_splits2;
    test_utils::SplitStr(output, "-", &new_splits2);

    EXPECT_EQ(new_splits[0], new_splits2[0]);
    EXPECT_EQ(new_splits[1], new_splits2[1]);
    EXPECT_EQ(new_splits[2], new_splits2[2]);
  }

}
}
