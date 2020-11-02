#include "resmack/types.hpp"
#include "resmack/rand.hpp"

namespace resmack {
namespace fuzz {

  // Returns the new index into orig_ss that iteration should continue at
  inline size_t CascadeMutatedState(uint32_t new_state[],
                          RandSnapshot* item,
                          size_t item_idx,
                          Vector<RandSnapshot>* orig_ss,
                          Vector<RandSnapshot>* new_ss) {
    new_ss->emplace_back(item->ref_depth, new_state);

    size_t idx;
    for (idx = item_idx + 1; idx <= orig_ss->size(); idx++) {
      RandSnapshot& curr_snapshot = (*orig_ss)[idx];
      if (curr_snapshot.ref_depth <= item->ref_depth) {
        break;
      }
    }
    return idx;
  }

  void MutateRandSnapshot(Rand* rand,
                          Vector<RandSnapshot>* orig_ss,
                          Vector<RandSnapshot>* new_ss) {
    size_t curr_idx = 0;
    RandSnapshot* curr = NULL;

    while (curr_idx < orig_ss->size()) {
      curr = &(*orig_ss)[curr_idx];
      if (rand->Maybe()) {
        curr_idx = CascadeMutatedState(rand->GetState(), curr, curr_idx, orig_ss, new_ss);
      } else {
        new_ss->emplace_back(curr->ref_depth, curr->state);
        curr_idx += 1;
      }
    }
  }

}
}
