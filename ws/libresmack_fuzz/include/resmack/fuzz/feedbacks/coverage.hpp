#ifndef RESMACK_FUZZ_COVERAGE_H
#define RESMACK_FUZZ_COVERAGE_H

#include <atomic>
#include <stdint.h>
#include <stdlib.h>
#include <semaphore.h>

#include "resmack/debug.hpp"
#include "resmack/fuzz/asan_util.hpp"
#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/lock.hpp"

namespace resmack {
namespace fuzz {

  ATTRIBUTE_NO_SANITIZING
  void HandleSanitizerCovTracePcGuardInit(uint32_t* start, uint32_t* stop);
  ATTRIBUTE_NO_SANITIZING
  void HandleSanitizerCovTracePcGuard(uint32_t* guard);

  static Lock NEW_COV_LOCK;
  static size_t GUARD_COUNTER = 0;
  static std::atomic<uint64_t>** COV_FLAGS = NULL;
  static size_t NUM_COV_FLAGS;
  static std::atomic<bool>* HAS_NEW_COV = NULL;

  class Coverage : public Feedback {
    private:
      std::atomic<bool> has_new;
      size_t hash;
      void* shared;
      // only updated when something new is found
      Lock shared_cov_lock;
      std::atomic<uint64_t>* cov_flags;
      std::atomic<uint64_t>* shared_cov_flags;

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
