#include "rand.hpp"
#include <cstddef>
#include <cstring>


namespace resmack {

  RandSnapshot::RandSnapshot(size_t ref_depth, uint32_t state[]) {
    this->ref_depth = ref_depth;
    memcpy(this->state, state, sizeof(uint32_t) * 4);
  }

  Rand::Rand() {
    std::random_device rd;
    this->Init(rd());
  }

  Rand::Rand(uint32_t seed) {
    this->Init(seed);
  }

  uint32_t Rand::Next() {
    const uint32_t result = this->Rotl(this->s_[1] * 5, 7) * 9;

    const uint32_t t = this->s_[1] << 9;

    this->s_[2] ^= this->s_[0];
    this->s_[3] ^= this->s_[1];
    this->s_[1] ^= this->s_[2];
    this->s_[0] ^= this->s_[3];

    this->s_[2] ^= t;

    this->s_[3] = this->Rotl(this->s_[3], 11);

    return result;
  }

  bool Rand::Maybe() {
    return this->Next() % 2 == 0;
  }

  void Rand::SnapshotState(size_t ref_depth) {
    this->snapshots_.emplace_back(ref_depth, this->s_);
  }

  void Rand::SnapshotClear() {
    this->snapshots_.clear();
  }

  void Rand::Init(uint32_t seed) {
    this->should_record_ = false;

    std::mt19937 gen;
    gen.seed(seed);

    s_[0] = gen();
    s_[1] = gen();
    s_[2] = gen();
    s_[3] = gen();
  }

  void Rand::SetState(uint32_t state[]) {
    memcpy(this->s_, state, sizeof(uint32_t) * 4);
  }

  inline uint32_t Rand::Rotl(const uint32_t x, int k) {
    return (x << k) | (x >> (32 - k));
  }
}
