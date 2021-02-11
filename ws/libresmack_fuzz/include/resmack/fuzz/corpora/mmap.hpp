#ifndef RESMACK_FUZZ_CORPORA_MMAP_H
#define RESMACK_FUZZ_CORPORA_MMAP_H

#include <algorithm>
#include <inttypes.h>
#include <semaphore.h>

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
    uint32_t strats;
    Vector<size_t(*)(MmapCorpus*, Rand*, size_t)> strat_handlers;
    sem_t* corpus_lock;
    Set<size_t> seen_keys;
    Vector<CorpusEntry> snapshots;
    Vector<size_t> most_direct_descendants_desc;
    Vector<size_t> most_descendants_desc;
    Vector<size_t> most_ancestors_desc;
    Vector<size_t> most_feedback;

    ser::CorpusMetadata* meta;
    size_t next_item_index;
    ser::CorpusItemHeader* first_item;
    ser::CorpusItemHeader* next_item;
    uint32_t last_updated_seq; 
    uint32_t last_reorg_seq; 

    size_t last_item1_one_based_idx;
    size_t last_item2_one_based_idx; // only used if crossover between two parents was used

    size_t corpus_decay;

    // pointer to the current number of iterations. READ ONLY!
    // WILL NOT BE EXACT! Iteration counts are synced every X intervals, not
    // with each iteration
    const size_t* curr_iter_count;
    size_t last_discovered_iteration;

   public:
    MmapCorpus();
    ~MmapCorpus();

    void SetCorpusDecay(size_t corpus_decay) {
      this->corpus_decay = corpus_decay;
    }

    void SetCurrIterPtr(size_t* curr_iter_count) {
      this->curr_iter_count = curr_iter_count;
    }

    float GetDecayPercent();

    void SetStrats(uint32_t strats);
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
    Vector<RandSnapshot>* GetItem(Rand* rand, size_t* last_idx1, size_t* last_idx2);
    void Sync();
    void SyncCounters();
    void SyncCountersInner();
    bool SeenFeedback(size_t feedback_key) { return this->seen_keys.contains(feedback_key); }
    size_t NumItems() { return this->snapshots.size(); }
    float GetUsedCapacity() {
      size_t used = (char*)this->next_item - (char*)this->corpus_map;
      return (float)used / (float)this->max_corpus_size;
    };
    size_t NumItemsRaw() { return this->meta->num_entries; }
    size_t ItersSinceNewItem() {
      return *this->curr_iter_count - this->last_discovered_iteration;
    }
    void IncLastItemCrashes();
    void IncUnwanted(size_t one_based_idx);

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

    static size_t HandleRandStrat(MmapCorpus* this_, Rand* rand, size_t rand_top_ten);
    static size_t HandleMostFeedbackStrat(MmapCorpus* this_, Rand* rand, size_t rand_top_ten);
    static size_t HandleLeastFeedbackStrat(MmapCorpus* this_, Rand* rand, size_t rand_top_ten);
    static size_t HandleMostRecentStrat(MmapCorpus* this_, Rand* rand, size_t rand_top_ten);
    static size_t HandleLeastRecentStrat(MmapCorpus* this_, Rand* rand, size_t rand_top_ten);
    static size_t HandleMostAncestorsStrat(MmapCorpus* this_, Rand* rand, size_t rand_top_ten);
    static size_t HandleLeastAncestorsStrat(MmapCorpus* this_, Rand* rand, size_t rand_top_ten);
    static size_t HandleMostDirectDescendantsStrat(MmapCorpus* this_, Rand* rand, size_t rand_top_ten);
    static size_t HandleLeastDirectDescendantsStrat(MmapCorpus* this_, Rand* rand, size_t rand_top_ten);
    static size_t HandleMostDescendantsStrat(MmapCorpus* this_, Rand* rand, size_t rand_top_ten);
    static size_t HandleLeastDescendantsStrat(MmapCorpus* this_, Rand* rand, size_t rand_top_ten);
  };

}
}
}

#endif
