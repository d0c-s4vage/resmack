#ifndef RESMACK_FUZZ_CORPORA_MMAP_H
#define RESMACK_FUZZ_CORPORA_MMAP_H

#include "inttypes.h"
#include "semaphore.h"

#include "resmack/fuzz/corpus.hpp"
#include "resmack/types.hpp"
#include "resmack/fuzz/serialized.hpp"

namespace resmack {
namespace fuzz {
namespace corpora {

class MmapCorpus : public Corpus {
 private:
  void* corpus_map;
  size_t max_corpus_size;
  sem_t* corpus_lock;
  Vector<Vector<RandSnapshot>> snapshots;
  Set<size_t> seen_keys;

  ser::CorpusMetadata* meta;
  size_t next_item_index;
  ser::CorpusItemHeader* next_item;
  uint32_t last_updated_seq; 
  uint32_t last_reorg_seq; 

  // pointer to the current number of iterations. READ ONLY!
  // WILL NOT BE EXACT! Iteration counts are synced every X intervals, not
  // with each iteration
  const size_t* curr_iter_count;
  size_t last_discovered_iteration;

 public:
  MmapCorpus();
  ~MmapCorpus();

  void SetCurrIterPtr(size_t* curr_iter_count) {
    this->curr_iter_count = curr_iter_count;
  }

  void Init(void* corpus_map, size_t max_corpus_size);
  void AddRandSnapshot(const resmack::Vector<RandSnapshot>* snapshot, size_t feedback_key);
  bool AddRandSnapshotIfNotSeen(const resmack::Vector<RandSnapshot>* snapshot, size_t feedback_key);
  Vector<RandSnapshot>* GetItem(Rand* rand);
  void Sync();
  bool SeenFeedback(size_t feedback_key) { return this->seen_keys.contains(feedback_key); }
  size_t NumItems() { return this->snapshots.size(); }
  size_t ItersSinceNewItem() {
    return *this->curr_iter_count - this->last_discovered_iteration;
  }

 private:
  void AddRandSnapshotInner(const resmack::Vector<RandSnapshot>* snapshot, size_t feedback_key);
  void SyncInner();
};

}
}
}

#endif
