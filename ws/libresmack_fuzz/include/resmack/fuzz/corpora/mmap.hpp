#ifndef RESMACK_FUZZ_CORPORA_MMAP_H
#define RESMACK_FUZZ_CORPORA_MMAP_H

#include "inttypes.h"
#include "semaphore.h"

#include "resmack/types.hpp"
#include "resmack/fuzz/corpus.hpp"
#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/serialized.hpp"

namespace resmack {
namespace fuzz {
namespace corpora {

class MmapCorpus : public Corpus {
 private:
  void* corpus_map;
  size_t max_corpus_size;
  sem_t* corpus_lock;
  Set<size_t> seen_keys;
  Vector<CorpusEntry> snapshots;
  Vector<size_t> most_direct_descendants_desc;
  Vector<size_t> most_descendants_desc;
  Vector<size_t> most_ancestors_desc;
  Vector<size_t> most_crashes_desc;
  Vector<size_t> most_feedback;

  ser::CorpusMetadata* meta;
  size_t next_item_index;
  ser::CorpusItemHeader* first_item;
  ser::CorpusItemHeader* next_item;
  uint32_t last_updated_seq; 
  uint32_t last_reorg_seq; 

  size_t last_item1_one_based_idx;
  size_t last_item2_one_based_idx; // only used if crossover between two parents was used

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

  void Init(const char* state_path, void* corpus_map, size_t max_corpus_size);
  void AddRandSnapshot(
    const resmack::Vector<RandSnapshot>* snapshot,
    FeedbackStats stats,
    bool descendant_of_last
  );
  bool AddRandSnapshotIfNotSeen(
    const resmack::Vector<RandSnapshot>* snapshot,
    FeedbackStats stats,
    bool descendant_of_last
  );
  virtual const Vector<CorpusEntry>* GetItems() { return &this->snapshots; }
  Vector<RandSnapshot>* GetItem(Rand* rand);
  void Sync();
  bool SeenFeedback(size_t feedback_key) { return this->seen_keys.contains(feedback_key); }
  size_t NumItems() { return this->snapshots.size(); }
  size_t NumItemsRaw() { return this->meta->num_entries; }
  size_t ItersSinceNewItem() {
    return *this->curr_iter_count - this->last_discovered_iteration;
  }
  void IncLastItemCrashes();

 private:
  void AddRandSnapshotInner(
    const resmack::Vector<RandSnapshot>* snapshot,
    FeedbackStats stats,
    bool descendant_of_last
  );
  void SyncInner();
  size_t UpdateStats(CorpusEntry* entry, size_t level);
  ser::CorpusItemHeader* GetItemHeader(size_t index);
  void SortedsAdd(size_t index);
  void SortedsClear();
  void SortedsResort();
};

}
}
}

#endif
