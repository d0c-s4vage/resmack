#ifndef RESMACK_FUZZ_SERIALIZED_H
#define RESMACK_FUZZ_SERIALIZED_H

#include "stddef.h"
#include "inttypes.h"

namespace resmack {
namespace fuzz {
namespace ser {

struct GenState {
  uint32_t ref_depth;
  uint32_t max_depth;
  uint32_t rule_idx;
  uint32_t rand_state[4];
};

struct GenStateHeader {
  uint64_t num_states;
};

struct CorpusItemHeader {
  uint64_t size;
  uint64_t feedback_key;
  uint64_t feedback_num; // *something* where higher == more coverage
  uint64_t iter_discovered;
  uint64_t parent1_one_based_idx; // 1-based!!! 0 == no parent
  uint64_t parent2_one_based_idx;
  uint64_t num_crashes;
  uint64_t num_ancestors;
  uint64_t num_descendants;
  uint64_t num_direct_descendants;
  uint64_t reserved1;
  uint64_t reserved2;
  GenStateHeader item_header;
};

struct CorpusMetadata {
  // number that gets incremented every time the corpus is updated
  uint32_t updated_seq;
  uint32_t reorg_seq;
  uint32_t num_entries;
};

struct AsanInfo {
  uint32_t exists; // bool
  char major_hash[41];
  char minor_hash[41];
  char report[0x10000];
  char stack[0x10000];
};

}
} // namespace fuzz
} // namespace resmack

#endif
