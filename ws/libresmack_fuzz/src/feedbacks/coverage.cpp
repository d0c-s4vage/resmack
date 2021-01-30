#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <semaphore.h>
#include <sys/mman.h>

#include "resmack/fuzz/feedbacks/coverage.hpp"
#include "resmack/fuzz/ipc_util.hpp"

namespace resmack {
namespace fuzz {

  static size_t GUARD_COUNTER = 0;
  static uint32_t* COV_FLAGS = NULL;
  static uint32_t* SHARED_COV_FLAGS = NULL;
  static size_t NUM_COV_FLAGS;
  static bool IS_NEW = false;

  void HandleSanitizerCovTracePcGuardInit(uint32_t* start, uint32_t* stop) {
    if (start == stop || *start) return;  // Initialize only once.
    for (uint32_t *x = start; x < stop; x++) {
      *x = ++GUARD_COUNTER;  // Guards should start from 1.
    }
    NUM_COV_FLAGS = (GUARD_COUNTER - 1) / 32;
    if ((GUARD_COUNTER - 1) % 32 != 0) {
      NUM_COV_FLAGS++;
    }
    printf("Total Edges: %zu\n", GUARD_COUNTER);
  }

  void HandleSanitizerCovTracePcGuard(uint32_t* guard) {
    if (COV_FLAGS == NULL) { return; }

    size_t uint_no = (*guard - 1) / 32;
    size_t bit_no = (*guard - 1) % 32;
    size_t bit = (1 << bit_no);

    if (COV_FLAGS[uint_no] & bit) { return; }
    COV_FLAGS[uint_no] |= bit;

    IS_NEW = true;
  }

  size_t _NumBits(uint32_t val) {
     val = val - ((val >> 1) & 0x55555555);
     val = (val & 0x33333333) + ((val >> 2) & 0x33333333);
     return (((val + (val >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
  }

  // --------------------------------------------------------------------------

  Coverage::Coverage() {
    this->shared = mmap(
      NULL,
      sizeof(sem_t) + NUM_COV_FLAGS * sizeof(uint32_t),
      PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS,
      -1,
     0
    );

    if (this->shared == MAP_FAILED) {
      perror("Could not create coverage mmap");
      std::exit(1);
    }

    this->cov_lock = (sem_t*)this->shared;
    if (sem_init(this->cov_lock, 0, 1)) {
      perror("Could not init semaphore for coverage sharing");
      std::exit(1);
    }

    SHARED_COV_FLAGS = (uint32_t*)((char*)this->shared + sizeof(sem_t));
    COV_FLAGS = (uint32_t*)malloc(sizeof(uint32_t) * NUM_COV_FLAGS);
    memset(COV_FLAGS, 0, sizeof(uint32_t) * NUM_COV_FLAGS);
  }

  Coverage::~Coverage() {
    munmap(this->shared, NUM_COV_FLAGS * sizeof(uint32_t));
    free(COV_FLAGS);
  }

  std::string Coverage::GetSummary() {
    size_t total_bits = 0;
    for (size_t idx = 0; idx < NUM_COV_FLAGS; idx++) {
      total_bits += _NumBits(COV_FLAGS[idx]);
    }

    return std::to_string(total_bits) + " edges";
  }

  void Coverage::Start() {
    IS_NEW = false;
    //memset(COV_FLAGS, 0, sizeof(uint32_t) * NUM_COV_FLAGS);
  }

  void Coverage::Stop() {
    this->hash = 0;
    for (size_t idx = 0; idx < NUM_COV_FLAGS; idx++) {
      uint32_t x = COV_FLAGS[idx];
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

  void Coverage::Sync() {
    WITH_LOCK(this->cov_lock, Syncing coverage flags, {
      for (size_t i = 0; i < NUM_COV_FLAGS; i++) {
        SHARED_COV_FLAGS[i] |= COV_FLAGS[i];
      }
      memcpy(COV_FLAGS, SHARED_COV_FLAGS, sizeof(uint32_t) * NUM_COV_FLAGS);
    });
  }

  FeedbackStats Coverage::GetStats() {
    size_t num_bits = 0;
    for (size_t idx = 0; idx < NUM_COV_FLAGS; idx++) {
      num_bits += _NumBits(COV_FLAGS[idx]);
    }

    if (IS_NEW) {
      this->Sync();
    }

    return {
      .new_coverage = IS_NEW,
      .key = this->hash,
      .num = num_bits,
    };
  }

}
}
