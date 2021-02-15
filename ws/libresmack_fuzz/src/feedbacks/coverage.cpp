#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <mutex>
#include <stdio.h>
#include <semaphore.h>
#include <sys/mman.h>

#include "resmack/fuzz/feedbacks/coverage.hpp"
#include "resmack/fuzz/ipc/locked_shared_mem.hpp"
#include "resmack/fuzz/ipc_util.hpp"
#include "resmack/fuzz/target_hooks.hpp"

namespace resmack {
namespace fuzz {
namespace feedbacks {

  static size_t GUARD_COUNTER = 0;
  static size_t NUM_COV_UINT32;

  // only used in the target
  static uint32_t* IN_TARGET_COV_FLAGS = NULL;
  static bool* IS_NEW; // ptr to the ipc mem

  // only used outside of the target
  static std::mutex SHARED_COV_FLAGS_LOCK;;
  static uint32_t* SHARED_COV_FLAGS = NULL;

  void HandleSanitizerCovTracePcGuardInit(uint32_t* start, uint32_t* stop) {
    if (start == stop || *start) return;  // Initialize only once.
    for (uint32_t *x = start; x < stop; x++) {
      *x = ++GUARD_COUNTER;  // Guards should start from 1.
    }
    NUM_COV_UINT32 = (GUARD_COUNTER - 1) / 32;
    if ((GUARD_COUNTER - 1) % 32 != 0) {
      NUM_COV_UINT32++;
    }
    printf("Total Edges: %zu\n", GUARD_COUNTER);
  }

  void HandleSanitizerCovTracePcGuard(uint32_t* guard) {
    if (IN_TARGET_COV_FLAGS == NULL) { return; }

    size_t uint_no = (*guard - 1) / 32;
    size_t bit_no = (*guard - 1) % 32;
    size_t bit = (1 << bit_no);

    // this happens *IN* the forked process, no need to use the lock
    if (IN_TARGET_COV_FLAGS[uint_no] & bit) { return; }
    IN_TARGET_COV_FLAGS[uint_no] |= bit;
    *IS_NEW = true;
  }

  size_t _NumBits(uint32_t val) {
     val = val - ((val >> 1) & 0x55555555);
     val = (val & 0x33333333) + ((val >> 2) & 0x33333333);
     return (((val + (val >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
  }

  // --------------------------------------------------------------------------

  Coverage::Coverage() : cov_lock("coverage-lock") {
    SHARED_COV_FLAGS_LOCK.lock();
      if (SHARED_COV_FLAGS = NULL) {
        SHARED_COV_FLAGS = (uint32_t*)malloc(sizeof(uint32_t) * NUM_COV_UINT32);
        memset(SHARED_COV_FLAGS, 0, sizeof(uint32_t) * NUM_COV_UINT32);
      }
    SHARED_COV_FLAGS_LOCK.unlock();
  }

  Coverage::~Coverage() {
    SHARED_COV_FLAGS_LOCK.lock();
      if (SHARED_COV_FLAGS != NULL) {
        free(SHARED_COV_FLAGS);
        SHARED_COV_FLAGS = NULL;
      }
    SHARED_COV_FLAGS_LOCK.unlock();
  }

  std::string Coverage::GetSummary() {
    size_t total_bits = 0;
    for (size_t idx = 0; idx < NUM_COV_UINT32; idx++) {
      total_bits += _NumBits(SHARED_COV_FLAGS[idx]);
    }

    return std::to_string(total_bits) + " edges";
  }

  void Coverage::CalcHash() {
    this->hash = 0;
    uint32_t* cov_map = this->GetCovMap();
    for (size_t idx = 0; idx < NUM_COV_UINT32; idx++) {
      uint32_t x = cov_map[idx];
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

  void Coverage::SyncSharedToTarget() {
    SHARED_COV_FLAGS_LOCK.lock();
      uint32_t* cov_map = this->GetCovMap();
      memcpy(cov_map, SHARED_COV_FLAGS, sizeof(uint32_t) * NUM_COV_UINT32);
    SHARED_COV_FLAGS_LOCK.unlock();
  }

  void Coverage::SyncTargetToShared() {
    SHARED_COV_FLAGS_LOCK.lock();
      uint32_t* cov_map = this->GetCovMap();
      for (size_t i = 0; i < NUM_COV_UINT32; i++) {
        SHARED_COV_FLAGS[i] |= cov_map[i];
      }
    SHARED_COV_FLAGS_LOCK.unlock();
  }

  FeedbackStats Coverage::GetStats() {
    size_t num_bits = 0;
    uint32_t* cov_map = this->GetCovMap();
    for (size_t idx = 0; idx < NUM_COV_UINT32; idx++) {
      num_bits += _NumBits(cov_map[idx]);
    }

    return {
      .new_coverage = *IS_NEW,
      .key = this->hash,
      .num = num_bits,
    };
  }

  void Coverage::InsertHooks(TargetHooks* hooks) {
    size_t ipc_size =
      sizeof(CoverageIpcInfo) + sizeof(uint32_t) * (NUM_COV_UINT32 - 1);

    hooks
      ->AddIpcSize([ipc_size]() -> size_t { return ipc_size; })
      ->AddIpcInit([this, ipc_size](ipc::LockedSharedMem* mem) {
          this->ipc = mem->GetNextPtrFor<CoverageIpcInfo>(ipc_size);
        })
      ->AddPreStartInTarget([this](ipc::LockedSharedMem*) {
          IN_TARGET_COV_FLAGS = this->GetCovMap();
          IS_NEW = &this->ipc->is_new;
        })
      ->AddPreTest([this](ipc::LockedSharedMem*) {
          this->ipc->is_new = false;
        })
      ->AddPostTest([this](ipc::LockedSharedMem*) {
          if (this->ipc->is_new) {
            this->CalcHash();
            this->SyncTargetToShared();
          }
        });
  }

}
}
}
