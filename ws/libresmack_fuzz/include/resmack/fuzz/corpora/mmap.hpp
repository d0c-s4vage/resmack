#ifndef RESMACK_FUZZ_CORPORA_MMAP_H
#define RESMACK_FUZZ_CORPORA_MMAP_H

#include "inttypes.h"
#include "semaphore.h"

#include "resmack/fuzz/corpus.hpp"
#include "resmack/types.hpp"

namespace resmack {
namespace fuzz {
namespace corpora {

struct MmapCorpusRandState {
  uint32_t ref_depth;
  uint32_t rule_idx;
  uint32_t rand_state[4];
};

struct MmapCorpusItemHeader {
  uint32_t num_states;
  // total size of item including the header (header + rand state array)
  uint32_t size;
  uint32_t reserved1;
  uint32_t reserved2;
};

struct MmapMetadata {
  // number that gets incremented every time the corpus is updated
  uint32_t updated_seq;
  uint32_t reorg_seq;
  uint32_t num_entries;
};

class MmapCorpus : public Corpus {
 private:
  void* corpus_map;
  size_t max_corpus_size;
  Vector<Vector<RandSnapshot>> snapshots;
  sem_t* corpus_lock;

  MmapMetadata* meta;
  size_t next_item_index;
  MmapCorpusItemHeader* next_item;
  uint32_t last_updated_seq; 
  uint32_t last_reorg_seq; 

 public:
  MmapCorpus();
  ~MmapCorpus();

  void Init(void* corpus_map, size_t max_corpus_size);
  size_t NumItems() { return this->snapshots.size(); }
  void AddRandSnapshot(resmack::Vector<RandSnapshot>* snapshot, int num);
  Vector<RandSnapshot>* GetItem(Rand* rand);
  void Sync();
  void SyncInner();
};

}
}
}

#endif
