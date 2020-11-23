#ifndef RESMACK_FUZZ_STATE_H
#define RESMACK_FUZZ_STATE_H

#include "unistd.h"

#include "resmack/fuzz/corpus.hpp"

namespace resmack {
namespace fuzz {

class State {
  virtual uint64_t GetNumIterations() = 0;
  virtual void IncNumIterations() = 0;
  virtual void IncNumIterations(uint64_t amt) = 0;

  virtual uint64_t GetNumCrashes() = 0;
  virtual void IncNumCrashes() = 0;
  virtual void IncNumCrashes(uint64_t amt) = 0;

  virtual Corpus* GetCorpus() = 0;
};

}
}

#endif
