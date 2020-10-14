#ifndef RESMACK_RAND
#define RESMACK_RAND

#include <cstdlib>
#include <stdlib.h>
#include <stdint.h>
#include <random>

namespace resmack {

class Rand {
  private:
   uint32_t s_[4];

  public: 
   Rand() {
     std::random_device rd;
     this->Init(rd());
   }
   Rand(uint32_t seed) {
     this->Init(seed);
   }
   uint32_t Next() {
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
   bool Maybe() {
     return this->Next() % 2 == 0;
   }

  private:
   void Init(uint32_t seed) {
     std::mt19937 gen;
     gen.seed(seed);

     s_[0] = gen();
     s_[1] = gen();
     s_[2] = gen();
     s_[3] = gen();
   }
   static inline uint32_t Rotl(const uint32_t x, int k) {
     return (x << k) | (x >> (32 - k));
   }
};

}

#endif
