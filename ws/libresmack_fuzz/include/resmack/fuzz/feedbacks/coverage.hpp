#ifndef RESMACK_FUZZ_COVERAGE_H
#define RESMACK_FUZZ_COVERAGE_H

#include <functional>
#include <stdint.h>
#include <stdlib.h>
#include <semaphore.h>

#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/ipc/queued_shared_mem.hpp"
#include "resmack/fuzz/lock.hpp"
#include "resmack/fuzz/target_hooks.hpp"

namespace resmack {
namespace fuzz {
namespace feedbacks {

  void HandleSanitizerCovTracePcGuard(uint32_t* guard_var);
  void HandleSanitizerCovTracePcGuardInit(uint32_t* start, uint32_t* end);

  static uint16_t COV_UPDATE_TYPE = 0x10;
  static uint32_t* SHARED_COV_FLAGS = NULL;

  class Coverage;

  using NewCoverageCb = std::function<void(Coverage* cov)>;

  class Coverage : public Feedback {
   private:
    size_t hash;
    NewCoverageCb new_coverage_cb;

    void SyncTargetToShared();
    resmack::fuzz::ipc::QueuedSharedMem* queued_mem;

   public:
    Coverage(NewCoverageCb new_coverage_cb);
    ~Coverage();
    // Return a summary of the state of the SHARED_COV_FLAGS
    void Clear();
    void CalcHash();
    std::string GetSummary();
    FeedbackStats GetStats();

    void TestInitShared();
    void TestDestroyShared();

    void InsertHooks(TargetHooks* hooks);
  };

}
}
}

#endif
