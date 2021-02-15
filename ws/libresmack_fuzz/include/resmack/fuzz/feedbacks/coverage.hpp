#ifndef RESMACK_FUZZ_COVERAGE_H
#define RESMACK_FUZZ_COVERAGE_H

#include <stdint.h>
#include <stdlib.h>
#include <semaphore.h>

#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/lock.hpp"
#include "resmack/fuzz/target_hooks.hpp"

namespace resmack {
namespace fuzz {
namespace feedbacks {

  void HandleSanitizerCovTracePcGuard(uint32_t* guard_var);
  void HandleSanitizerCovTracePcGuardInit(uint32_t* start, uint32_t* end);

  struct CoverageIpcInfo {
    bool is_new;
    uint32_t cov_map;
  };

  class Coverage : public Feedback {
   private:
    size_t hash;
    // only updated when something new is found
    Lock cov_lock;
    CoverageIpcInfo *ipc;

    uint32_t* GetCovMap() { return &this->ipc->cov_map; }
    void SyncTargetToShared();
    void CalcHash();

   public:
    Coverage();
    ~Coverage();
    // Return a summary of the state of the SHARED_COV_FLAGS
    std::string GetSummary();
    void SyncSharedToTarget();
    FeedbackStats GetStats();

    void InsertHooks(TargetHooks* hooks);
  };

}
}
}

#endif
