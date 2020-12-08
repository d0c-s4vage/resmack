
#include <cstring>
#include <iostream>
#include <thread>

#include "resmack/rand.hpp"


namespace resmack {

  RandSnapshot::RandSnapshot(size_t ref_depth, uint32_t rule_idx, const uint32_t state[]) :
    ref_depth(ref_depth),
    rule_idx(rule_idx)
  {
    memcpy(this->state, state, sizeof(uint32_t) * 4);
  }

  Rand::Rand() {
    std::random_device rd;
    this->Init(rd());
  }

  Rand::Rand(uint32_t seed) {
    this->Init(seed);
  }

  Rand::Rand(const Rand& other) {
    this->should_record_ = other.should_record_;
    memcpy(this->s_, other.s_, sizeof(this->s_));
    for (const RandSnapshot& item: other.snapshots_) {
      this->snapshots_.emplace_back(item.ref_depth, item.rule_idx, item.state);
    }
  }

  Rand& Rand::operator=(const Rand& other) {
    if (this == &other) {
      return *this;
    }

    this->should_record_ = other.should_record_;
    memcpy(this->s_, other.s_, sizeof(this->s_));
    for (const RandSnapshot& item: other.snapshots_) {
      this->snapshots_.emplace_back(item.ref_depth, item.rule_idx, item.state);
    }
    return *this;
  }

  Rand::~Rand() {}

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

  void Rand::SnapshotState(size_t ref_depth, uint32_t rule_idx) {
    this->snapshots_.emplace_back(ref_depth, rule_idx, this->s_);
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

  void Rand::SetState(const uint32_t state[]) {
    memcpy(this->s_, state, sizeof(uint32_t) * 4);
  }

  void Rand::CopyState(uint32_t state[]) {
    memcpy(state, this->s_, sizeof(uint32_t) * 4);
  }

  inline uint32_t Rand::Rotl(const uint32_t x, int k) {
    return (x << k) | (x >> (32 - k));
  }
}
