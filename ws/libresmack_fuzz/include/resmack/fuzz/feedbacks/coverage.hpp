#ifndef RESMACK_FUZZ_COVERAGE_H
#define RESMACK_FUZZ_COVERAGE_H

#include <stdint.h>
#include <stdlib.h>
#include <semaphore.h>

#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/lock.hpp"
#include "resmack/fuzz/asan_util.hpp"

namespace resmack {
namespace fuzz {

  ATTRIBUTE_NO_SANITIZE_COVERAGE
  void HandleSanitizerCovTracePcGuard(uint32_t* guard_var);
  ATTRIBUTE_NO_SANITIZE_COVERAGE
  void HandleSanitizerCovTracePcGuardInit(uint32_t* start, uint32_t* end);

  class Coverage : public Feedback {
   private:
    size_t hash;
    void* shared;
    // only updated when something new is found
    Lock cov_lock;

   public:
    Coverage();
    ~Coverage();
    ATTRIBUTE_NO_SANITIZE_COVERAGE
    std::string GetSummary();
    ATTRIBUTE_NO_SANITIZE_COVERAGE
    void Start();
    ATTRIBUTE_NO_SANITIZE_COVERAGE
    void Stop();
    ATTRIBUTE_NO_SANITIZE_COVERAGE
    void Sync();
    ATTRIBUTE_NO_SANITIZE_COVERAGE
    FeedbackStats GetStats();
  };

}
}

#endif
