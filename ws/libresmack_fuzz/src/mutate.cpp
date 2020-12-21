#include <iostream>

#include "resmack/types.hpp"
#include "resmack/rand.hpp"

namespace resmack {
namespace fuzz {

  // Returns the new index into orig_ss that iteration should continue at
  __attribute__((noinline))
  size_t CascadeMutatedState(
    uint32_t* new_state,
    uint32_t new_max_depth,
    const RandSnapshot& item,
    size_t item_idx,
    const Vector<RandSnapshot>* orig_ss,
    Vector<RandSnapshot>* new_ss
  ) {
    new_ss->emplace_back(item.ref_depth, new_max_depth, item.rule_idx, new_state);

    size_t idx;
    for (idx = item_idx + 1; idx < orig_ss->size(); idx++) {
      const RandSnapshot& curr_snapshot = (*orig_ss)[idx];
      if (curr_snapshot.ref_depth <= item.ref_depth) {
        break;
      }
    }
    return idx;
  }

  void MutateRandSnapshot(
    Rand* rand,
    const Vector<RandSnapshot>* orig_ss,
    Vector<RandSnapshot>* new_ss,
    uint32_t total_max_depth
  ) {
    size_t curr_idx = 0;
    new_ss->clear();

    // skip the first index, and only mutate *ONE* at a time
    size_t idx_to_mutate = (rand->Next() % (orig_ss->size() - 1)) + 1;

    while (curr_idx < orig_ss->size()) {
      const RandSnapshot& curr = orig_ss->at(curr_idx);
      if (curr_idx == idx_to_mutate) {
        uint32_t new_max_depth = curr.max_depth;
        if (rand->Maybe()) {
          new_max_depth = rand->Next() % total_max_depth;
          if (rand->Maybe()) {
            new_ss->emplace_back(curr.ref_depth, new_max_depth, curr.rule_idx, curr.state);
            continue;
          }
        }
        //uint32_t range = total_max_depth - curr.max_depth;
        //uint32_t new_max_depth = rand->Next() % range + curr.max_depth;
        curr_idx = CascadeMutatedState(
          rand->GetState(),
          new_max_depth,
          curr,
          curr_idx,
          orig_ss,
          new_ss
        );
      } else {
        new_ss->emplace_back(curr.ref_depth, curr.max_depth, curr.rule_idx, curr.state);
        curr_idx += 1;
      }
    }
  }

}
}
