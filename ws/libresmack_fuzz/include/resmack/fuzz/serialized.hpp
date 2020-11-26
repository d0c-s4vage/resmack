#ifndef RESMACK_FUZZ_SERIALIZED_H
#define RESMACK_FUZZ_SERIALIZED_H

#include "stddef.h"
#include "inttypes.h"

namespace resmack {
namespace fuzz {
namespace ser {

struct GenState {
  uint32_t ref_depth;
  uint32_t rule_idx;
  uint32_t rand_state[4];
};

struct GenStateHeader {
  uint32_t num_states;
};

struct CorpusItemHeader {
  uint32_t size;
  size_t feedback_key;
  uint32_t reserved1;
  uint32_t reserved2;
  GenStateHeader item_header;
};

struct CorpusMetadata {
  // number that gets incremented every time the corpus is updated
  uint32_t updated_seq;
  uint32_t reorg_seq;
  uint32_t num_entries;
};

}
} // namespace fuzz
} // namespace resmack

#endif
