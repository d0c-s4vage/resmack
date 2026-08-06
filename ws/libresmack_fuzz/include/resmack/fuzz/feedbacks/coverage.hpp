#ifndef RESMACK_FUZZ_COVERAGE_H
#define RESMACK_FUZZ_COVERAGE_H

#include <atomic>
#include <memory>
#include <mm_malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>

#include "resmack/debug.hpp"
#include "resmack/fuzz/asan_util.hpp"
#include "resmack/fuzz/feedback.hpp"
#include "resmack/fuzz/lock.hpp"


extern "C" {
  __attribute__((visibility("default")))
  void __sanitizer_cov_trace_pc_guard_init(uint32_t* start, uint32_t* end);

  __attribute__((visibility("default")))
  void __sanitizer_cov_trace_pc_guard(uint32_t* guard_var);
}

namespace resmack {
namespace fuzz {
  struct CoverageData {
    bool fpo_saw_new;
    uint64_t hash;
    size_t num_flags;
    uint64_t hit_flags; 
    uint8_t flags[1];
  };

  inline constexpr size_t MAX_COV_FLAGS = 100'000'000;
  inline size_t NUM_COV_FLAGS = 0;
  inline size_t GUARD_COUNTER = 0;

  /**
   * These are used by the two PcGuard functions. They are used ONLY in the fuzzing
   * process!
   *
   * FPO == Fuzz Process Only
   */
  inline CoverageData* FPO_COV_DATA = nullptr;

  ATTRIBUTE_NO_SANITIZING
  void HandleSanitizerCovTracePcGuardInit(uint32_t* start, uint32_t* stop) noexcept;
  ATTRIBUTE_NO_SANITIZING
  void HandleSanitizerCovTracePcGuard(uint32_t* guard) noexcept;

  struct MMapDeleter {
    size_t size_;
    void operator()(void* p) const {
      if (!p || p == MAP_FAILED) { return; }
      munmap(p, size_);
    }
  };

  struct AlignedDeleter {
    void operator()(void* p) const {
      _mm_free(p);
    }
  };

  class Coverage : public Feedback {
    private:
      Lock sync_lock_;
      size_t cov_data_size_;

      // local coverage data for the fuzzing processes ONLY - NOT stored in the mmap
      std::unique_ptr<CoverageData, AlignedDeleter> fpo_data_;

      // not directly used by the fuzzing processes - it's synced to
      // from the fpo_data when new coverage is found.
      std::unique_ptr<CoverageData, MMapDeleter> shared_data_;

      // Calculates the hash on the fpo_data->flags
      uint64_t CalcHash();

    public:
      Coverage();
      ~Coverage();

      void FPOStart() noexcept;
      void FPOStop() noexcept;

      bool Sync() noexcept;
      FeedbackStats GetStats() const noexcept;
      std::string GetSummary() const noexcept;
  };

}
}

#endif
