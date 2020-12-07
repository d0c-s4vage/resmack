#ifndef RESMACK_FUZZ_CORPUS
#define RESMACK_FUZZ_CORPUS

#include "resmack/rand.hpp"

namespace resmack {
namespace fuzz {

struct CorpusEntry;

struct CorpusEntry {
  CorpusEntry* parent1;
  CorpusEntry* parent2;
  uint64_t descendant_level;
  uint64_t num_direct_descendants;
  uint64_t num_descendants;
  uint64_t num_mutations;
  uint64_t num_crashes;
  Vector<Vector<RandSnapshot>> snapshot;
};

class Corpus {
 public:
  virtual void AddRandSnapshot(const resmack::Vector<RandSnapshot>* snapshot, size_t feedback_key) = 0;
  virtual const Vector<Vector<RandSnapshot>>* GetItems() = 0;
  virtual Vector<RandSnapshot>* GetItem(Rand* rand) = 0;
  /**
   * Intended to be called on intervals to do "processing" (whatever that means
   * to the specific corpus implementation
   **/
  virtual void Sync() = 0;
  virtual size_t NumItems() = 0;
  virtual bool SeenFeedback(size_t key) = 0;
  virtual bool AddRandSnapshotIfNotSeen(
    const resmack::Vector<RandSnapshot>* snapshot,
    size_t key
  ) = 0;
  virtual size_t ItersSinceNewItem() = 0;

  // --------------------------------------------------------------------------

  virtual 
};

}
}


#endif
