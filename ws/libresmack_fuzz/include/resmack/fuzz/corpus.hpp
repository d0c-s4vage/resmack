#ifndef RESMACK_FUZZ_CORPUS
#define RESMACK_FUZZ_CORPUS

#include "resmack/rand.hpp"

namespace resmack {
namespace fuzz {

  class ICorpus {
    virtual void AddRandSnapshot(resmack::Vector<RandSnapshot>* snapshot, int num) = 0;
    virtual void GetItem(Rand* rand) = 0;
  };

}
}


#endif
