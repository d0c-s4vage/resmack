#ifndef RESMACK_RAND
#define RESMACK_RAND

#include <sys/types.h>
#include <cstdlib>
#include <stdlib.h>
#include <stdint.h>
#include <random>

#include "types.hpp"

namespace resmack {

  class RandSnapshot {
    public:
      size_t ref_depth;
      uint32_t state[4];

      RandSnapshot(size_t ref_depth, uint32_t state[]);
  };

  class Rand {
    private:
     uint32_t s_[4];
     Vector<RandSnapshot> snapshots_;
     bool should_record_;

    public: 
     Rand();
     Rand(uint32_t seed);
     uint32_t Next();
     Vector<RandSnapshot>* GetSnapshots() { return &this->snapshots_; }
     bool Maybe();
     void SnapshotState(size_t ref_depth);
     void SnapshotClear();
     inline void SetShouldRecord(bool val) { this->should_record_ = val; }
     inline bool ShouldRecord() { return this->should_record_; }
     void SetState(uint32_t state[]);
     void FetchState(uint32_t state[]);

    private:
     void Init(uint32_t seed);
     static inline uint32_t Rotl(const uint32_t x, int k);
  };

}

#endif
