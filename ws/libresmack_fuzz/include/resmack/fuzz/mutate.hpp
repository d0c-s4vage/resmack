#ifndef RESMACK_FUZZ_MUTATE
#define RESMACK_FUZZ_MUTATE

#include "resmack/types.hpp"
#include "resmack/rand.hpp"

namespace resmack {
namespace fuzz {

  // Returns the new index into orig_ss that iteration should continue at
  __attribute__((visibility("default")))
  size_t CascadeMutatedState(uint32_t new_state[],
                          uint32_t new_max_depth,
                          const RandSnapshot& item,
                          size_t item_idx,
                          const Vector<RandSnapshot>* orig_ss,
                          Vector<RandSnapshot>* new_ss);

  __attribute__((visibility("default")))
  void MutateRandSnapshot(Rand* rand,
                          const Vector<RandSnapshot>* orig_ss,
                          Vector<RandSnapshot>* new_ss,
                          uint32_t total_max_depth);

}
}

#endif
