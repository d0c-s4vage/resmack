#include <cstring>
#include <iostream>
#include <math.h>
#include <thread>
#include <signal.h>

#include "resmack/rand.hpp"


namespace resmack {

  RandSnapshot::RandSnapshot(
      uint32_t ref_depth,
      uint32_t max_depth,
      uint32_t rule_idx,
      const uint32_t state[]
  ) :
    ref_depth(ref_depth),
    max_depth(max_depth),
    rule_idx(rule_idx)
  {
    memcpy(this->state, state, sizeof(uint32_t) * 4);
  }

  Rand::Rand() {
    this->ReinitSeed();
  }

  Rand::Rand(uint32_t seed) {
    this->Init(seed);
  }

  Rand::Rand(const Rand& other) {
    this->should_record_ = other.should_record_;
    memcpy(this->s_, other.s_, sizeof(this->s_));
    for (const RandSnapshot& item: other.snapshots_) {
      this->snapshots_.emplace_back(
        item.ref_depth,
        item.max_depth,
        item.rule_idx,
        item.state
      );
    }
  }

  Rand& Rand::operator=(const Rand& other) {
    if (this == &other) {
      return *this;
    }

    this->should_record_ = other.should_record_;
    memcpy(this->s_, other.s_, sizeof(this->s_));
    for (const RandSnapshot& item: other.snapshots_) {
      this->snapshots_.emplace_back(
        item.ref_depth,
        item.max_depth,
        item.rule_idx,
        item.state
      );
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

  uint32_t Rand::NextInRangeGaussian(uint32_t min_val, uint32_t max_val) {
    double mean = ((double)max_val + (double)min_val) / 2.0;
    double standard_dev = double(max_val - min_val) / 8.0;

    float tmp;
    if (this->use_last_gaussian) {
      tmp = this->last_gaussian;
      this->use_last_gaussian = !this->use_last_gaussian;
    } else {
      double u, v, s;
      do {
        //std::cout << "next_raw: " << this->Next() << " next: " << (double)this->Next() << " max: " << (double)std::numeric_limits<uint32_t>::max() << std::endl;
        u = ((double)this->Next() / (double)std::numeric_limits<uint32_t>::max()) * 2.0 - 1.0;
        v = ((double)this->Next() / (double)std::numeric_limits<uint32_t>::max()) * 2.0 - 1.0;
        s = u * u + v * v;
        //std::cout << "u: " << u << " v: " << v << " s: " << s << std::endl;
      } while (s >= 1.0 || s == 0.0);

      s = sqrt(-2.0 * log(s) / s);
      this->last_gaussian = v * s;
      tmp = u * s;
      this->use_last_gaussian = true;
    }

    double res = mean + standard_dev * tmp;
    if (res < min_val || res > max_val) {
      return this->NextInRangeGaussian(min_val, max_val);
    }
    return res;
  }

  void Rand::SnapshotState(
      uint32_t ref_depth,
      uint32_t max_depth,
      uint32_t rule_idx
  ) {
    this->snapshots_.emplace_back(ref_depth, max_depth, rule_idx, this->s_);
  }

  void Rand::SnapshotClear() {
    this->snapshots_.clear();
  }

  void Rand::Init(uint32_t seed) {
    this->should_record_ = false;
    this->InitState(seed);
  }

  void Rand::ReinitSeed() {
    std::random_device rd;
    this->Init(rd());
  }

  void Rand::InitState(uint32_t seed) {
    std::mt19937 gen;
    gen.seed(seed);

    s_[0] = gen();
    s_[1] = gen();
    s_[2] = gen();
    s_[3] = gen();
  }

  void Rand::SetState(const uint32_t state[]) {
    this->use_last_gaussian = false;
    memcpy(this->s_, state, sizeof(uint32_t) * 4);
    if (this->s_[0] == 0 && this->s_[1] == 0 && this->s_[2] == 0 && this->s_[3] == 0) {
      raise(SIGINT);
    }
  }

  void Rand::CopyState(uint32_t state[]) {
    memcpy(state, this->s_, sizeof(uint32_t) * 4);
  }

  inline uint32_t Rand::Rotl(const uint32_t x, int k) {
    return (x << k) | (x >> (32 - k));
  }
}
