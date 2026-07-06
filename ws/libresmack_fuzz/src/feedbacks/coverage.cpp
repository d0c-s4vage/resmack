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
  size_t _NumBits(uint32_t val) {
     val = val - ((val >> 1) & 0x55555555);
     val = (val & 0x33333333) + ((val >> 2) & 0x33333333);
     return (((val + (val >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
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
      DEBUG_PRINT("Could not create coverage mmap: NUM_COV_FLAGS: %zu", NUM_COV_FLAGS);
      perror("Could not create coverage mmap");
      std::exit(1);
    }

    this->cov_flags = NULL;
    COV_FLAGS = &this->cov_flags;

    this->shared_cov_flags = (uint32_t*)this->shared;
    memset(this->shared_cov_flags, 0, sizeof(uint32_t) * NUM_COV_FLAGS);
  }

  Coverage::~Coverage() {
    munmap(this->shared, NUM_COV_FLAGS * sizeof(uint32_t));
    {
      DEBUG_PRINT("Coverage::~Coverage Waiting for scoped lock\n");
      std::scoped_lock lock(NEW_COV_MUTEX);
      DEBUG_PRINT("Coverage::~Coverage got it\n");
      free(this->cov_flags);
      this->cov_flags = NULL;
      COV_FLAGS = NULL;
      DEBUG_PRINT("Coverage::~Coverage DONE\n");
    }
  }

  std::string Coverage::GetSummary() {
    size_t total_bits = 0;
    for (size_t idx = 0; idx < NUM_COV_FLAGS; idx++) {
      total_bits += _NumBits(this->cov_flags[idx]);
    }

    return std::to_string(total_bits) + " edges";
  }

  void Coverage::Start() {
    {
      std::scoped_lock lock(NEW_COV_MUTEX);
      IS_NEW = false;
      this->cov_flags = (uint32_t*)malloc(sizeof(uint32_t) * NUM_COV_FLAGS);
      memset(this->cov_flags, 0, sizeof(uint32_t) * NUM_COV_FLAGS);
    }
  }

  void Coverage::Stop() {
    {
      std::scoped_lock lock(NEW_COV_MUTEX);
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
  }

  bool Coverage::Sync() {
    bool was_new = false;
    {
      std::scoped_lock lock(NEW_COV_MUTEX, this->shared_cov_lock);
      if (IS_NEW) {
        // copy our changes to the shared coverage flags
        for (size_t i = 0; i < NUM_COV_FLAGS; i++) {
          this->shared_cov_flags[i] |= this->cov_flags[i];
        }
        // now pull in any coverage flags that were seen by
        // others
        memcpy(this->cov_flags, this->shared_cov_flags, sizeof(uint32_t) * NUM_COV_FLAGS);
      }
      was_new = IS_NEW;
      IS_NEW = false;
    }
    return was_new;
  }

  FeedbackStats Coverage::GetStats() {
    size_t num_bits = 0;
    for (size_t idx = 0; idx < NUM_COV_FLAGS; idx++) {
      num_bits += _NumBits(this->cov_flags[idx]);
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
