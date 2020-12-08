#ifndef RESMACK_FUZZ_CORPUS
#define RESMACK_FUZZ_CORPUS

#include "resmack/rand.hpp"

namespace resmack {
namespace fuzz {

struct CorpusEntry;

struct CorpusEntry {
  size_t parent1_one_based_idx; // 0 == NOT SET
  size_t parent2_one_based_idx; // 0 == NOT SET
  uint64_t index;
  uint64_t iter_discovered;
  uint64_t descendant_level;
  uint64_t num_ancestors;
  uint64_t num_direct_descendants;
  uint64_t num_descendants;
  uint64_t num_crashes;
  Vector<RandSnapshot> snapshot;

  CorpusEntry() :
    parent1_one_based_idx(0),
    parent2_one_based_idx(0),
    index(0),
    descendant_level(0),
    num_descendants(0),
    num_crashes(0)
  {}
};

class Corpus {
 public:
  virtual void AddRandSnapshot(
    const resmack::Vector<RandSnapshot>* snapshot,
    size_t feedback_key,
    bool descendant_of_last
  ) = 0;
  virtual bool AddRandSnapshotIfNotSeen(
    const resmack::Vector<RandSnapshot>* snapshot,
    size_t key,
    bool descendant_of_last
  ) = 0;
  virtual const Vector<CorpusEntry>* GetItems() = 0;
  virtual Vector<RandSnapshot>* GetItem(Rand* rand) = 0;
  /**
   * Intended to be called on intervals to do "processing" (whatever that means
   * to the specific corpus implementation
   **/
  virtual void Sync() = 0;
  virtual size_t NumItems() = 0;
  virtual bool SeenFeedback(size_t key) = 0;
  virtual size_t ItersSinceNewItem() = 0;
  virtual void IncLastItemCrashes() = 0;
};

}
}


#endif
