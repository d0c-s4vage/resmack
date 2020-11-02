#ifndef RESMACK_FUZZ_CORPUS
#define RESMACK_FUZZ_CORPUS

#include "resmack/rand.hpp"

namespace resmack {
namespace fuzz {

  class ICorpus {
    virtual void AddItem() = 0;
    virtual void GetItem(Rand* rand) = 0;
  };

  class Corpus : ICorpus {
  };

}
}


#endif
