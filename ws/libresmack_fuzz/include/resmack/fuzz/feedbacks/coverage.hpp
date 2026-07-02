#ifndef RESMACK_FUZZ_COVERAGE_H
#define RESMACK_FUZZ_COVERAGE_H

#include <mutex>
#include <thread>
#include <stdint.h>
#include <stdlib.h>
#include <semaphore.h>

#include "resmack/debug.hpp"
#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/lock.hpp"
#include "resmack/fuzz/asan_util.hpp"

namespace resmack {
namespace fuzz {
  static inline std::mutex NEW_COV_MUTEX;
  static size_t GUARD_COUNTER = 0;
  static uint32_t** COV_FLAGS = NULL;
  static size_t NUM_COV_FLAGS;
  static bool IS_NEW = false;

  ATTRIBUTE_NO_SANITIZE_COVERAGE
  __attribute__((no_sanitize("coverage")))
  inline void HandleSanitizerCovTracePcGuardInit(uint32_t* start, uint32_t* stop) {
    if (start == stop || *start) return;  // Initialize only once.
    for (uint32_t *x = start; x < stop; x++) {
      *x = ++GUARD_COUNTER;  // Guards should start from 1.
    }
    NUM_COV_FLAGS = (GUARD_COUNTER - 1) / 32;
    if ((GUARD_COUNTER - 1) % 32 != 0) {
      NUM_COV_FLAGS++;
    }
  }

  inline void _sanitizer_print_guard_source(uint32_t* guard) {
       void *pc = __builtin_return_address(0);
       char pc_desc[1024];
       __sanitizer_symbolize_pc(pc, "%p %F %L", pc_desc, sizeof(pc_desc));
       printf("guard: %p %x PC %s\n", guard, *guard, pc_desc);
  }

  ATTRIBUTE_NO_SANITIZE_COVERAGE
  __attribute__((no_sanitize("coverage")))
  inline void HandleSanitizerCovTracePcGuard(uint32_t* guard) {
    // _sanitizer_print_guard_source(guard);
    {
      std::scoped_lock lock(NEW_COV_MUTEX);
      uint32_t* flags;
      if (NULL == COV_FLAGS || NULL == (flags = *COV_FLAGS)) { return; }

      size_t uint_no = (*guard - 1) / 32;
      size_t bit_no = (*guard - 1) % 32;
      size_t bit = (1 << bit_no);

      if (flags[uint_no] & bit) { return; }

      flags[uint_no] |= bit;
      IS_NEW = true;
    }
  }


  class Coverage : public Feedback {
   private:
    size_t hash;
    void* shared;
    // only updated when something new is found
    Lock shared_cov_lock;
    uint32_t *cov_flags;
    uint32_t *shared_cov_flags;

   public:
    Coverage();
    ~Coverage();

    ATTRIBUTE_NO_SANITIZING
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
