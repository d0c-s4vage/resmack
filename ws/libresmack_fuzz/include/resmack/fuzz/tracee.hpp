#ifndef RESMACK_FUZZ_TRACEE_H
#define RESMACK_FUZZ_TRACEE_H

#include <inttypes.h>
#include <time.h>
#include <unistd.h>

#include "resmack/rand.hpp"
#include "resmack/fuzz/serialized.hpp"

#define TRACEE_MAX_LAST_GEN_STATES 0x10000

namespace resmack {
namespace fuzz {

struct TraceeShared {
  float iter_start_;
  float lifetime_start_;
  size_t last_corpus_index1_;
  size_t last_corpus_index2_;
  size_t last_max_depth_;
  bool last_used_corpus_; 
  ser::GenStateHeader last_gen_state_;
  ser::GenState states_[sizeof(ser::GenState) * TRACEE_MAX_LAST_GEN_STATES];
};

class Tracee {
  uint32_t idx_;
  // mmap'd shared space for IPC communication
  TraceeShared* basic_shared_;
  ser::AsanInfo* asan_shared_;

 public:
  bool iter_timed_out_;
  bool lifetime_timed_out_;
  Tracee(uint32_t idx);
  ~Tracee();

  void Reset();
  uint32_t GetIdx() { return this->idx_; }

  void SaveAsanInfo(const char* report);
  const ser::AsanInfo* GetAsanInfo() {
    if (!this->asan_shared_->exists) { return nullptr; }
    return this->asan_shared_;
  }

  void SaveLastCorpusInfo(bool used_corpus, size_t last_corpus_idx1, size_t last_corpus_idx2, size_t max_depth);
  size_t GetLastCorpusIndex1() { return this->basic_shared_->last_corpus_index1_; }
  size_t GetLastCorpusIndex2() { return this->basic_shared_->last_corpus_index2_; }
  size_t GetLastMaxDepth() { return this->basic_shared_->last_max_depth_; }
  bool GetLastUsedCorpus() { return this->basic_shared_->last_used_corpus_; }
  void IterStart();
  float GetIterStart();
  float GetLifetimeStart();

  void SaveLastReplay(Vector<RandSnapshot>* replay);
  void LoadLastReplay(Vector<RandSnapshot>* dest);
};

}
}

#endif
