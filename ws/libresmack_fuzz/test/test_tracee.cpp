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
    snapshots.emplace_back(900, 1000, state);

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
    EXPECT_EQ(snapshots.size(), 1u);
    EXPECT_EQ(snapshots[0].ref_depth, 900u);
    EXPECT_EQ(snapshots[0].rule_idx, 1000u);
    EXPECT_EQ(snapshots[0].state[0], 1u);
    EXPECT_EQ(snapshots[0].state[1], 2u);
    EXPECT_EQ(snapshots[0].state[2], 3u);
    EXPECT_EQ(snapshots[0].state[3], 4u);
  }

}
}
