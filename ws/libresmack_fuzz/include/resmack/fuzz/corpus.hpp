#ifndef RESMACK_FUZZ_CORPUS
#define RESMACK_FUZZ_CORPUS

#include "resmack/rand.hpp"

namespace resmack {
namespace fuzz {

class Corpus {
 public:
  virtual void AddRandSnapshot(resmack::Vector<RandSnapshot>* snapshot, int num) = 0;
  virtual Vector<RandSnapshot>* GetItem(Rand* rand) = 0;
  /**
   * Intended to be called on intervals to do "processing" (whatever that means
   * to the specific corpus implementation
   **/
  virtual void Sync() = 0;
  virtual size_t NumItems() = 0;
};

}
}


#endif
