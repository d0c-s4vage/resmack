#ifndef RESMACK_FUZZ_CORPUS
#define RESMACK_FUZZ_CORPUS

#include "resmack/rand.hpp"
#include "resmack/fuzz/feedback.hpp"

namespace resmack {
namespace fuzz {

  enum class CorpusStrat : uint32_t {
    kRand                   = 0b1,
    kMostFeedback           = 0b10,
    kLeastFeedback          = 0b100,
    kMostRecent             = 0b1000,
    kLeastRecent            = 0b10000,
    kMostAncestors          = 0b100000,
    kLeastAncestors         = 0b1000000,
    kMostDirectDescendants  = 0b10000000,
    kLeastDirectDescendants = 0b100000000,
    kMostDescendants        = 0b1000000000,
    kLeastDescendants       = 0b10000000000,
  };

  struct CorpusEntry {
    size_t parent1_one_based_idx; // 0 == NOT SET
    size_t parent2_one_based_idx; // 0 == NOT SET
    uint64_t index;
    uint64_t mutations_since_offspring_new;
    uint64_t mutations_since_offspring;
    uint64_t feedback_key;
    uint64_t feedback_num;
    uint64_t iter_discovered;
    uint64_t num_ancestors;
    uint64_t num_direct_descendants;
    uint64_t num_descendants;
    uint64_t num_crashes;
    float decay_pct;
    Vector<RandSnapshot> snapshot;

    CorpusEntry() :
      parent1_one_based_idx(0),
      parent2_one_based_idx(0),
      index(0),
      mutations_since_offspring_new(0),
      mutations_since_offspring(0),
      feedback_num(0),
      iter_discovered(0),
      num_ancestors(0),
      num_direct_descendants(0),
      num_descendants(0),
      num_crashes(0),
      decay_pct(1.0)
    {}
  };

  class Corpus {
   public:
    virtual void AddRandSnapshot(
      const resmack::Vector<RandSnapshot>* snapshot,
      feedbacks::FeedbackStats stats,
      bool descendant_of_last
    ) = 0;
    virtual bool AddRandSnapshotIfNotSeen(
      const resmack::Vector<RandSnapshot>* snapshot,
      feedbacks::FeedbackStats stats,
      bool descendant_of_last
    ) = 0;
    virtual const Vector<CorpusEntry>* GetItems() = 0;
    virtual Vector<RandSnapshot>* GetItem(Rand* rand, size_t* last_idx1, size_t* last_idx2) = 0;
    /**
     * Intended to be called on intervals to do "processing" (whatever that means
     * to the specific corpus implementation
     **/
    virtual void Sync() = 0;
    virtual size_t NumItems() = 0;
    virtual size_t NumItemsRaw() = 0;
    virtual bool SeenFeedback(size_t key) = 0;
    virtual size_t ItersSinceNewItem() = 0;
    virtual void IncLastItemCrashes() = 0;
    virtual void IncUnwanted(size_t one_based_idx) = 0;
    virtual float GetUsedCapacity() = 0;
  };

}
}


#endif
