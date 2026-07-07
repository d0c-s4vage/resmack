#ifndef RESMACK_FUZZ_COVERAGE_H
#define RESMACK_FUZZ_COVERAGE_H

#include <mutex>
#include <thread>
#include <stdint.h>
#include <stdlib.h>
#include <semaphore.h>

#include "resmack/debug.hpp"
#include "resmack/fuzz/asan_util.hpp"
#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/lock.hpp"

namespace resmack {
namespace fuzz {

  void HandleSanitizerCovTracePcGuardInit(uint32_t* start, uint32_t* stop);
  void HandleSanitizerCovTracePcGuard(uint32_t* guard);

  static inline std::mutex NEW_COV_MUTEX;
  static size_t GUARD_COUNTER = 0;
  static uint32_t** COV_FLAGS = NULL;
  static size_t NUM_COV_FLAGS;
  static bool IS_NEW = false;

  class Coverage : public Feedback {
   private:
    size_t hash;
    void* shared;
    // only updated when something new is found
    Lock shared_cov_lock;
    uint32_t *cov_flags;
    uint32_t *shared_cov_flags;

    void CalcHash();

   public:
    ATTRIBUTE_NO_SANITIZING
    Coverage();
    ATTRIBUTE_NO_SANITIZING
    ~Coverage();

    std::string GetSummary();
    ATTRIBUTE_NO_SANITIZING
    void Start();
    ATTRIBUTE_NO_SANITIZING
    void Stop();
    ATTRIBUTE_NO_SANITIZING
    bool Sync();
    ATTRIBUTE_NO_SANITIZING
    FeedbackStats GetStats();
  };

}
}

#endif
