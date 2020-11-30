#ifndef RESMACK_FUZZ_CORPUS
#define RESMACK_FUZZ_CORPUS

#include "resmack/rand.hpp"

namespace resmack {
namespace fuzz {

class Corpus {
 public:
  virtual void AddRandSnapshot(const resmack::Vector<RandSnapshot>* snapshot, size_t feedback_key) = 0;
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
};

}
}


#endif
