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
      uint32_t ref_depth;
      uint32_t max_depth;
      uint32_t rule_idx;
      uint32_t state[4];

      RandSnapshot(
        uint32_t ref_depth,
        uint32_t max_depth,
        uint32_t rule_idx,
        const uint32_t state[]
      );
  };

  class Rand {
    private:
     uint32_t s_[4];
     Vector<RandSnapshot> snapshots_;
     bool should_record_;
     double last_gaussian;
     bool use_last_gaussian;

    public: 
     Rand();
     ~Rand();
     Rand(const Rand& other);
     Rand& operator=(const Rand& other);
     Rand(uint32_t seed);

     uint32_t Next();
     uint32_t NextInRangeGaussian(uint32_t min_val, uint32_t max_val);
     void ReinitSeed();
     void InitState(uint32_t seed);
     const Vector<RandSnapshot>* GetSnapshots() const { return &this->snapshots_; }
     bool Maybe();
     void SnapshotState(uint32_t ref_depth, uint32_t max_depth, uint32_t rule_idx);
     void SnapshotClear();
     inline void SetShouldRecord(bool val) { this->should_record_ = val; }
     inline bool ShouldRecord() { return this->should_record_; }
     void SetState(const uint32_t state[]);
     uint32_t* GetState() { return this->s_; }
     void CopyState(uint32_t state[]);

    private:
     void Init(uint32_t seed);
     static inline uint32_t Rotl(const uint32_t x, int k);
  };

}

#endif
