#include "gtest/gtest.h"

#include "resmack/rand.hpp"
#include "resmack/fuzz/mutate.hpp"

namespace resmack {
namespace fuzz {

  TEST(Mutate, MutateSnapshots) {
    Rand rand(100);
    rand.SnapshotClear();
    rand.Next();
    rand.SnapshotState(0, 5, 0);
    rand.Next();
    rand.SnapshotState(1, 5, 0);
    rand.Next();
    rand.SnapshotState(2, 5, 0);
    rand.Next();
    rand.SnapshotState(1, 5, 0);
    rand.Next();
    rand.SnapshotState(0, 5, 0);
    rand.Next();
    rand.SnapshotState(0, 5, 0);

    Vector<RandSnapshot> new_state_tree;
    const Vector<RandSnapshot>* orig_state_tree = rand.GetSnapshots();
    uint32_t tmp_state[4] = { 0, 1, 2, 3};

    size_t tests[][2] = {
      { 0u, 4u },
      { 1u, 3u },
      { 2u, 3u },
      { 3u, 4u },
      { 4u, 5u },
    };

    for (const auto& test: tests) {
      size_t cascade_idx = test[0];
      size_t returned_idx = test[1];
      new_state_tree.clear();

      size_t res_idx = resmack::fuzz::CascadeMutatedState(
        tmp_state,
        10,
        (*orig_state_tree)[cascade_idx],
        cascade_idx,
        (const Vector<RandSnapshot>*)orig_state_tree,
        &new_state_tree
      );
      EXPECT_EQ(res_idx, returned_idx);
    }
  }

}
}
