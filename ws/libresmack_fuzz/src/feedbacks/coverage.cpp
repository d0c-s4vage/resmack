#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <semaphore.h>
#include <mutex>
#include <thread>
#include <sys/mman.h>

#include "resmack/fuzz/feedbacks/coverage.hpp"
#include "resmack/fuzz/ipc_util.hpp"

namespace resmack {
namespace fuzz {
  void HandleSanitizerCovTracePcGuardInit(uint32_t* start, uint32_t* stop) {
    if (start == stop || *start) return;  // Initialize only once.
    for (uint32_t *x = start; x < stop; x++) {
      *x = ++GUARD_COUNTER;  // Guards should start from 1.
    }
    NUM_COV_FLAGS = (GUARD_COUNTER - 1) / 32;
    if ((GUARD_COUNTER - 1) % 32 != 0) {
      NUM_COV_FLAGS++;
    }
  }

  /*
  void _sanitizer_print_guard_source(uint32_t* guard) {
       void *pc = __builtin_return_address(0);
       char pc_desc[1024];
       __sanitizer_symbolize_pc(pc, "%p %F %L", pc_desc, sizeof(pc_desc));
       printf("guard: %p %x PC %s\n", guard, *guard, pc_desc);
  }
  */

  void HandleSanitizerCovTracePcGuard(uint32_t* guard) {
    {
      std::scoped_lock lock(NEW_COV_MUTEX);
      uint32_t* flags;
      if (NULL == COV_FLAGS) { return; }

      flags = *COV_FLAGS;
      if (NULL == flags) { return; }

      size_t uint_no = (*guard - 1) / 32;
      size_t bit_no = (*guard - 1) % 32;
      size_t bit = (1 << bit_no);

      // we've seen this edge before
      if (flags[uint_no] & bit) {
        return;
      }

      flags[uint_no] |= bit;
    }
  }



  // --------------------------------------------------------------------------

  Coverage::Coverage() : shared_cov_lock("coverage-lock") {
    size_t num_flags_to_alloc = NUM_COV_FLAGS ? NUM_COV_FLAGS : 1; 
    this->shared = mmap(
      NULL,
      num_flags_to_alloc * sizeof(uint32_t),
      PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS,
      -1,
      0
    );

    if (this->shared == MAP_FAILED) {
      throw std::runtime_error("Could not create coverage mmap: " + std::string(std::strerror(errno)));
    }

    this->cov_flags = (uint32_t*)malloc(sizeof(uint32_t) * NUM_COV_FLAGS);
    memset(this->cov_flags, 0, sizeof(uint32_t) * NUM_COV_FLAGS);
    COV_FLAGS = &this->cov_flags;

    this->shared_cov_flags = (uint32_t*)this->shared;
    memset(this->shared_cov_flags, 0, sizeof(uint32_t) * NUM_COV_FLAGS);
  }

  Coverage::~Coverage() {
    munmap(this->shared, NUM_COV_FLAGS * sizeof(uint32_t));
    {
      std::scoped_lock lock(NEW_COV_MUTEX);
      free(this->cov_flags);
      this->cov_flags = NULL;
      COV_FLAGS = NULL;
    }
  }

  std::string Coverage::GetSummary() {
    if (this->cov_flags != NULL) {
      size_t total_bits = 0;
      for (size_t idx = 0; idx < NUM_COV_FLAGS; idx++) {
        total_bits += std::popcount(this->cov_flags[idx]);
      }
      return std::to_string(total_bits) + " edges";
    }

    return "? edges";
  }

  void Coverage::Start() {
    {
      std::scoped_lock lock(NEW_COV_MUTEX);
      COV_FLAGS = &this->cov_flags;
    }
  }

  void Coverage::Stop() {
    {
      std::scoped_lock lock(NEW_COV_MUTEX);
      COV_FLAGS = NULL;
      this->CalcHash();
    }
  }

  void Coverage::CalcHash() {
    this->hash = 0;
    for (size_t idx = 0; idx < NUM_COV_FLAGS; idx++) {
      uint32_t x = this->cov_flags[idx];
      x = ((x >> 16) ^ x) * 0x45d9f3b;
      x = ((x >> 16) ^ x) * 0x45d9f3b;
      x = (x >> 16) ^ x;
      if ((idx & 1) == 0) {
        this->hash ^= x;
      } else {
        this->hash ^= ((size_t)x << 32);
      }
    }
  }

  bool Coverage::Sync() {
    bool was_new = false;
    {
      std::scoped_lock lock(NEW_COV_MUTEX, this->shared_cov_lock);
      // copy our changes to the shared coverage flags
      for (size_t i = 0; i < NUM_COV_FLAGS; i++) {
        if (std::popcount(~this->shared_cov_flags[i] & this->cov_flags[i]) > 0) {
          was_new = true;
        }
        this->shared_cov_flags[i] |= this->cov_flags[i];
      }
      // now pull in any coverage flags that were seen by
      // others
      memcpy(this->cov_flags, this->shared_cov_flags, sizeof(uint32_t) * NUM_COV_FLAGS);
    }
    return was_new;
  }

  FeedbackStats Coverage::GetStats() {
    size_t num_bits = 0;
    {
      std::scoped_lock lock(NEW_COV_MUTEX, this->shared_cov_lock);
      for (size_t idx = 0; idx < NUM_COV_FLAGS; idx++) {
        num_bits += std::popcount(this->cov_flags[idx]);
      }
    }

    bool had_new = this->Sync();

    return {
      .new_coverage = had_new,
      .key = this->hash,
      .num = num_bits,
    };
  }

}
}
