#ifndef RESMACK_FUZZ_STATE_H
#define RESMACK_FUZZ_STATE_H

#include <algorithm>
#include <functional>
#include <unistd.h>

#include "resmack/fuzz/corpus.hpp"
#include "resmack/fuzz/target.hpp"

namespace resmack {
namespace fuzz {

using UniqueCrashCb = std::function<bool()>;

struct StateStats {
#define STAT(NAME) double duration_##NAME;
#include "resmack/fuzz/stats.def"
#undef STAT
};

class State {
 public:
  virtual StateStats* GetStats() = 0;
  virtual void SyncStats(TargetStats* stats) = 0;

  virtual uint64_t GetNumIterations() = 0;
  virtual void IncNumIterations() = 0;
  virtual void IncNumIterations(uint64_t amt) = 0;

  virtual uint64_t GetNumCrashes() = 0;
  virtual void IncNumCrashes() = 0;
  virtual void IncNumCrashes(uint64_t amt) = 0;
  virtual void IncNumCrashesIfTrue(UniqueCrashCb cb) = 0;

  virtual Corpus* GetCorpus() = 0;
};

}
}

#endif
