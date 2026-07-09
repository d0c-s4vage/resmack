#include <atomic>
#include <mm_malloc.h>
#include <cstring>
#include <fcntl.h>
#include <stdio.h>
#include <semaphore.h>
#include <mutex>
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
    NUM_COV_FLAGS = (GUARD_COUNTER - 1) / sizeof(*COV_FLAGS[0]);
    if ((GUARD_COUNTER - 1) % sizeof(*COV_FLAGS[0]) != 0) {
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
      std::atomic<uint64_t>* flags;
      if (NULL == COV_FLAGS) { return; }

      flags = *COV_FLAGS;
      if (NULL == flags) { return; }

      size_t uint_no = (*guard - 1) / (sizeof(flags[0]) * CHAR_BIT);
      size_t bit_no = (*guard - 1) % (sizeof(flags[0]) * CHAR_BIT);
      size_t bit = (1ULL << bit_no);
      uint64_t before = flags[uint_no].fetch_or(bit, std::memory_order_relaxed);
      if ((before & bit) == 0) {
        HAS_NEW_COV->store(true, std::memory_order_relaxed);        
      }
    }
  }



  // --------------------------------------------------------------------------

  Coverage::Coverage() : has_new(false), shared_cov_lock("coverage-lock") {
    size_t num_flags_to_alloc = NUM_COV_FLAGS ? NUM_COV_FLAGS : 1; 
    this->shared = mmap(
      NULL,
      num_flags_to_alloc * sizeof(this->cov_flags[0]),
      PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS,
      -1,
      0
    );

    if (this->shared == MAP_FAILED) {
      throw std::runtime_error("Could not create coverage mmap: " + std::string(std::strerror(errno)));
    }

    this->cov_flags = static_cast<std::atomic<uint64_t>*>(_mm_malloc(sizeof(this->cov_flags[0]) * NUM_COV_FLAGS, 64));
    memset(this->cov_flags, 0, sizeof(this->cov_flags[0]) * NUM_COV_FLAGS);
    COV_FLAGS = &this->cov_flags;

    this->shared_cov_flags = static_cast<std::atomic<uint64_t>*>(this->shared);
    memset(this->shared_cov_flags, 0, sizeof(this->cov_flags[0]) * NUM_COV_FLAGS);

    HAS_NEW_COV = &this->has_new;
  }

  Coverage::~Coverage() {
    munmap(this->shared, NUM_COV_FLAGS * sizeof(this->cov_flags[0]));
    COV_FLAGS = NULL;
    _mm_free(this->cov_flags);
    this->cov_flags = NULL;
  }

  std::string Coverage::GetSummary() {
    if (this->cov_flags != NULL) {
      size_t total_bits = 0;
      for (size_t idx = 0; idx < NUM_COV_FLAGS; idx++) {
        total_bits += std::popcount(this->cov_flags[idx].load(std::memory_order_relaxed));
      }
      return std::to_string(total_bits) + " edges";
    }

    return "? edges";
  }

  void Coverage::Start() {
    this->has_new.store(false);
    COV_FLAGS = &this->cov_flags;
  }

  void Coverage::Stop() {
    COV_FLAGS = NULL;
    this->CalcHash();
  }

  void Coverage::CalcHash() {
    this->hash = 0;
    for (size_t idx = 0; idx < NUM_COV_FLAGS; idx++) {
      uint64_t x = this->cov_flags[idx].load(std::memory_order_relaxed);
      x = ((x >> 32) ^ x) * 0x45d9f3b5b5a4bcaeULL;
      x = ((x >> 32) ^ x) * 0x45d9f3b5b5a4bcaeULL;
      x = (x >> 32) ^ x;
      this->hash ^= x;
    }
  }

  bool Coverage::Sync() {
    bool was_new = false;
    for (size_t i = 0; i < NUM_COV_FLAGS; i++) {
        uint64_t priv   = this->cov_flags[i].load(std::memory_order_relaxed);
        uint64_t before = this->shared_cov_flags[i].fetch_or(priv, std::memory_order_acq_rel);
        uint64_t merged = before | priv;

        was_new |= (priv & ~before) != 0;
        this->cov_flags[i] = merged;
    }
    //memcpy(this->cov_flags, this->shared_cov_flags, sizeof(this->cov_flags[0]) * NUM_COV_FLAGS);
    this->has_new.store(was_new);
    return was_new;
  }

  FeedbackStats Coverage::GetStats() {
    size_t num_bits = 0;
    for (size_t idx = 0; idx < NUM_COV_FLAGS; idx++) {
      num_bits += std::popcount(this->cov_flags[idx].load(std::memory_order_relaxed));
    }

    bool had_new = this->has_new.exchange(false, std::memory_order_relaxed);
    if (had_new) {
      this->Sync();
    }

    return {
      .new_coverage = had_new,
      .key = this->hash,
      .num = num_bits,
    };
  }

}
}
