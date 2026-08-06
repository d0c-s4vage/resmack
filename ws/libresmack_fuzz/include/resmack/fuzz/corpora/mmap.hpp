#ifndef RESMACK_FUZZ_CORPORA_MMAP_H
#define RESMACK_FUZZ_CORPORA_MMAP_H

#include <algorithm>
#include <atomic>
#include <inttypes.h>
#include <semaphore.h>
#include <filesystem>

#include "resmack/types.hpp"
#include "resmack/fuzz/corpus.hpp"
#include "resmack/fuzz/lock.hpp"
#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/serialized.hpp"

namespace fs = std::filesystem;

namespace resmack {
namespace fuzz {
namespace corpora {

  enum CorpusStrat : uint32_t {
    STRAT_RAND                     = 0b1,
    STRAT_MOST_FEEDBACK            = 0b10,
    STRAT_LEAST_FEEDBACK           = 0b100,
    STRAT_MOST_RECENT              = 0b1000,
    STRAT_LEAST_RECENT             = 0b10000,
    STRAT_MOST_ANCESTORS           = 0b100000,
    STRAT_LEAST_ANCESTORS          = 0b1000000,
    STRAT_MOST_DIRECT_DESCENDANTS  = 0b10000000,
    STRAT_LEAST_DIRECT_DESCENDANTS = 0b100000000,
    STRAT_MOST_DESCENDANTS         = 0b1000000000,
    STRAT_LEAST_DESCENDANTS        = 0b10000000000,
  };

  class MmapCorpus : public Corpus {
   private:
    void* corpus_map;
    size_t max_corpus_size;
    uint32_t strats;
    Vector<CorpusStrat> strat_handlers;
    Lock corpus_lock;
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
    const std::atomic<uint64_t>* curr_iter_count;
    size_t last_discovered_iteration;

   public:
    MmapCorpus(fs::path state_path);
    ~MmapCorpus();

    void SetCorpusDecay(size_t corpus_decay) {
      this->corpus_decay = corpus_decay;
    }

    void SetCurrIterPtr(std::atomic<uint64_t>* curr_iter_count) {
      this->curr_iter_count = curr_iter_count;
    }

    float GetDecayPercent();

    void SetStrats(uint32_t strats);
    void Init(void* corpus_map, size_t max_corpus_size);
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
      return this->curr_iter_count->load() - this->last_discovered_iteration;
    }
    void IncLastItemCrashes();
    void IncUnwanted(size_t one_based_idx);

   private:
    size_t GetRandIdxFromStrats(Rand* rand);

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

    size_t HandleRandStrat(Rand* rand, size_t rand_top_ten);
    size_t HandleMostFeedbackStrat(Rand* rand, size_t rand_top_ten);
    size_t HandleLeastFeedbackStrat(Rand* rand, size_t rand_top_ten);
    size_t HandleMostRecentStrat(Rand* rand, size_t rand_top_ten);
    size_t HandleLeastRecentStrat(Rand* rand, size_t rand_top_ten);
    size_t HandleMostAncestorsStrat(Rand* rand, size_t rand_top_ten);
    size_t HandleLeastAncestorsStrat(Rand* rand, size_t rand_top_ten);
    size_t HandleMostDirectDescendantsStrat(Rand* rand, size_t rand_top_ten);
    size_t HandleLeastDirectDescendantsStrat(Rand* rand, size_t rand_top_ten);
    size_t HandleMostDescendantsStrat(Rand* rand, size_t rand_top_ten);
    size_t HandleLeastDescendantsStrat(Rand* rand, size_t rand_top_ten);
  };

}
}
}

#endif
