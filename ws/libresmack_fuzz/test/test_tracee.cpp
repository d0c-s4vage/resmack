#include <cstdio>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <inttypes.h>

#include "gtest/gtest.h"

#include "resmack/rand.hpp"
#include "resmack/fuzz/tracee.hpp"

namespace resmack {
namespace fuzz {

  TEST(Tracee, CreatesIPCViaMmap) {
    Tracee t;

    Vector<RandSnapshot> snapshots;
    uint32_t state[4] = {1, 2, 3, 4};
    snapshots.emplace_back(900, 50, 1000, state);
    snapshots.emplace_back(901, 51, 1001, state);
    snapshots.emplace_back(902, 52, 1002, state);

    pid_t pid;
    if ((pid = fork()) == 0) {
      t.SaveLastCorpusInfo(true, 555, 444);
      t.SaveLastReplay(&snapshots);
      std::exit(0);
    } else {
      int status;
      waitpid(pid, &status, 0);
    }

    EXPECT_EQ(t.GetLastCorpusIndex(), 555u);
    EXPECT_EQ(t.GetLastMaxDepth(), 444u);
    EXPECT_EQ(t.GetLastUsedCorpus(), true);

    snapshots.clear();
    t.LoadLastReplay(&snapshots);
    EXPECT_EQ(snapshots.size(), 3u);

    EXPECT_EQ(snapshots[0].ref_depth, 900u);
    EXPECT_EQ(snapshots[0].max_depth, 50u);
    EXPECT_EQ(snapshots[0].rule_idx, 1000u);
    EXPECT_EQ(snapshots[0].state[0], 1u);
    EXPECT_EQ(snapshots[0].state[1], 2u);
    EXPECT_EQ(snapshots[0].state[2], 3u);
    EXPECT_EQ(snapshots[0].state[3], 4u);

    EXPECT_EQ(snapshots[1].ref_depth, 901u);
    EXPECT_EQ(snapshots[1].max_depth, 51u);
    EXPECT_EQ(snapshots[1].rule_idx, 1001u);
    EXPECT_EQ(snapshots[1].state[0], 1u);
    EXPECT_EQ(snapshots[1].state[1], 2u);
    EXPECT_EQ(snapshots[1].state[2], 3u);
    EXPECT_EQ(snapshots[1].state[3], 4u);

    EXPECT_EQ(snapshots[2].ref_depth, 902u);
    EXPECT_EQ(snapshots[2].max_depth, 52u);
    EXPECT_EQ(snapshots[2].rule_idx, 1002u);
    EXPECT_EQ(snapshots[2].state[0], 1u);
    EXPECT_EQ(snapshots[2].state[1], 2u);
    EXPECT_EQ(snapshots[2].state[2], 3u);
    EXPECT_EQ(snapshots[2].state[3], 4u);
  }

}
}
