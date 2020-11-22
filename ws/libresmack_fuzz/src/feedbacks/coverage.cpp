#include <cstring>
#include <stdio.h>
#include <iostream>

#include "resmack/fuzz/feedbacks/coverage.hpp"

static size_t GUARD_COUNTER = 0;
static uint32_t* COV_FLAGS;
static size_t NUM_COV_FLAGS;

extern "C" void __sanitizer_cov_trace_pc_guard_init(uint32_t* start, uint32_t* stop) {
  if (start == stop || *start) return;  // Initialize only once.
  for (uint32_t *x = start; x < stop; x++) {
    *x = ++GUARD_COUNTER;  // Guards should start from 1.
  }
  NUM_COV_FLAGS = (GUARD_COUNTER - 1) / 32;
  if ((GUARD_COUNTER - 1) % 32 != 0) {
    NUM_COV_FLAGS++;
  }
  COV_FLAGS = new uint32_t[NUM_COV_FLAGS];
}

extern "C" void __sanitizer_cov_trace_pc_guard(uint32_t* guard) {
  size_t uint_no = (*guard - 1) / 32;
  size_t bit_no = (*guard - 1) % 32;
  if (bit_no != 0) {
    uint_no++;
  }
  COV_FLAGS[uint_no] |= (1 << bit_no);
}

size_t _NumBits(uint32_t val) {
   val = val - ((val >> 1) & 0x55555555);
   val = (val & 0x33333333) + ((val >> 2) & 0x33333333);
   return (((val + (val >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

namespace resmack {
namespace fuzz {

  void Coverage::Start() {
    memset(COV_FLAGS, 0, sizeof(uint32_t) * NUM_COV_FLAGS);
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

  FeedbackStats Coverage::GetStats() {
    size_t num_bits = 0;
    for (size_t idx = 0; idx < NUM_COV_FLAGS; idx++) {
      num_bits += _NumBits(COV_FLAGS[idx]);
    }

    return {
      .key = this->hash,
      .num = num_bits,
    };
  }

}
}
