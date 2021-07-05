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

  // only used in the target - temporary flag difference
  static uint32_t* IN_TARGET_COV_FLAGS = NULL;
  static bool IS_NEW = true; // ptr to the ipc mem


  void HandleSanitizerCovTracePcGuardInit(uint32_t* start, uint32_t* stop) {
    if (start == stop || *start) return;  // Initialize only once.
    for (uint32_t *x = start; x < stop; x++) {
      *x = ++GUARD_COUNTER;  // Guards should start from 1.
    }
    NUM_COV_UINT32 = (GUARD_COUNTER - 1) / 32;
    if ((GUARD_COUNTER - 1) % 32 != 0) {
      NUM_COV_UINT32++;
    }

    size_t cov_flags_size = sizeof(uint32_t) * NUM_COV_UINT32;
    IN_TARGET_COV_FLAGS = (uint32_t*)malloc(cov_flags_size);
    memset(IN_TARGET_COV_FLAGS, 0, cov_flags_size);

    printf("Total Edges: %zu\n", GUARD_COUNTER);
  }

  void HandleSanitizerCovTracePcGuard(uint32_t* guard) {
    if (IN_TARGET_COV_FLAGS == NULL) { return; }

    size_t uint_no = (*guard - 1) / 32;
    size_t bit_no = (*guard - 1) % 32;
    size_t bit = (1 << bit_no);

    // We've seen this one before! Either in the shared flags, or in our
    // flags. Most common use case is the shared flags
    if (SHARED_COV_FLAGS[uint_no] & bit) { return; }
    if (IN_TARGET_COV_FLAGS[uint_no] & bit) { return; }

    IN_TARGET_COV_FLAGS[uint_no] |= bit;
    IS_NEW = true;
  }

  size_t _NumBits(uint32_t val) {
     val = val - ((val >> 1) & 0x55555555);
     val = (val & 0x33333333) + ((val >> 2) & 0x33333333);
     return (((val + (val >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
  }

  // --------------------------------------------------------------------------

  Coverage::Coverage() : queued_mem(NULL) {
  }

  Coverage::~Coverage() {
    if (IN_TARGET_COV_FLAGS != NULL) {
      free(IN_TARGET_COV_FLAGS);
      IN_TARGET_COV_FLAGS = NULL;
    }
  }

  void Coverage::Clear() {
    memset(IN_TARGET_COV_FLAGS, 0, sizeof(uint32_t) * NUM_COV_UINT32);
    IS_NEW = false;
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
    for (size_t idx = 0; idx < NUM_COV_UINT32; idx++) {
      uint32_t x = IN_TARGET_COV_FLAGS[idx];
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

  void Coverage::SyncTargetToShared() {
    this->queued_mem->QueueUpdate(
      COV_UPDATE_TYPE,
      sizeof(uint32_t) * NUM_COV_UINT32,
      IN_TARGET_COV_FLAGS);
  }

  FeedbackStats Coverage::GetStats() {
    size_t num_bits = 0;
    for (size_t idx = 0; idx < NUM_COV_UINT32; idx++) {
      num_bits += _NumBits(SHARED_COV_FLAGS[idx]);
    }

    return {
      .new_coverage = IS_NEW, // ???? TODO
      .key = this->hash,
      .num = num_bits,
    };
  }

  void Coverage::TestInitShared() {
    SHARED_COV_FLAGS = reinterpret_cast<uint32_t*>(malloc(sizeof(uint32_t) * NUM_COV_UINT32));
  }

  void Coverage::TestDestroyShared() {
    free(SHARED_COV_FLAGS);
  }

  void Coverage::InsertHooks(TargetHooks* hooks) {
    size_t cov_flags_size = sizeof(uint32_t) * NUM_COV_UINT32;
    hooks
      ->AddIpcSize([cov_flags_size]() -> size_t { return cov_flags_size; })
      ->AddIpcInit([this, cov_flags_size](ipc::QueuedSharedMem* mem) {
          this->queued_mem = mem;
          SHARED_COV_FLAGS = mem->GetNextPtrFor<uint32_t>(cov_flags_size);

          mem->AddReceiveHandler(COV_UPDATE_TYPE, [](size_t data_length, void* data, ipc::LockedSharedMem*) {
            uint32_t* flag_updates = reinterpret_cast<uint32_t*>(data);
            if (data_length != (NUM_COV_UINT32 * sizeof(uint32_t))) {
              printf("SOMETHING WRONG HAPPENED WITH IPC UPDATE\n");
            }
            for (size_t i = 0; i < NUM_COV_UINT32 && ((i * sizeof(uint32_t)) <= data_length); i++) {
              SHARED_COV_FLAGS[i] |= *flag_updates;
              flag_updates++;
            }

            free(data);
          });
        })
      ->AddPreStartInTarget([](ipc::QueuedSharedMem*) {
          memset(IN_TARGET_COV_FLAGS, 0, sizeof(uint32_t) * NUM_COV_UINT32);
          IS_NEW = false;
        })
      ->AddPreTest([](ipc::QueuedSharedMem*) {
          IS_NEW = false;
        })
      ->AddPostTest([this](ipc::QueuedSharedMem*) {
          if (IS_NEW) {
            this->CalcHash();
            this->SyncTargetToShared();
          }
        });
  }

}
}
}
