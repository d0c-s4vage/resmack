#ifndef RESMACK_FUZZ_MUTATE
#define RESMACK_FUZZ_MUTATE

#include "resmack/types.hpp"
#include "resmack/rand.hpp"

namespace resmack {
namespace fuzz {

  // Returns the new index into orig_ss that iteration should continue at
  size_t CascadeMutatedState(uint32_t new_state[],
                          const RandSnapshot* item,
                          size_t item_idx,
                          const Vector<RandSnapshot>* orig_ss,
                          Vector<RandSnapshot>* new_ss);

  void MutateRandSnapshot(Rand* rand,
                          const Vector<RandSnapshot>* orig_ss,
                          Vector<RandSnapshot>* new_ss);

}
}

#endif
