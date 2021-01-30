#ifndef RESMACK_FUZZ_COVERAGE_H
#define RESMACK_FUZZ_COVERAGE_H

#include <stdint.h>
#include <stdlib.h>
#include <semaphore.h>

#include "resmack/fuzz/feedback.hpp"

namespace resmack {
namespace fuzz {
  void HandleSanitizerCovTracePcGuard(uint32_t* guard_var);
  void HandleSanitizerCovTracePcGuardInit(uint32_t* start, uint32_t* end);

  class Coverage : public Feedback {
   private:
    size_t hash;
    void* shared;
    // only updated when something new is found
    sem_t* cov_lock;

   public:
    Coverage();
    ~Coverage();
    std::string GetSummary();
    void Start();
    void Stop();
    void Sync();
    FeedbackStats GetStats();
  };

}
}

#endif
