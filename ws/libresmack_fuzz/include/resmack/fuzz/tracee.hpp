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
  float iter_start;
  float lifetime_start;
  size_t last_corpus_index1;
  size_t last_corpus_index2;
  size_t last_max_depth;
  bool last_used_corpus; 
  ser::GenStateHeader last_gen_state;
  ser::GenState states[sizeof(ser::GenState) * TRACEE_MAX_LAST_GEN_STATES];
};

class Tracee {
  uint32_t idx;
  // mmap'd shared space for IPC communication
  TraceeShared* basic_shared;
  ser::AsanInfo* asan_shared;

 public:
  bool iter_timed_out;
  bool lifetime_timed_out;
  Tracee(uint32_t idx);
  ~Tracee();

  void Reset();
  uint32_t GetIdx() { return this->idx; }

  void SaveAsanInfo(const char* report);
  const ser::AsanInfo* GetAsanInfo() {
    if (!this->asan_shared->exists) { return NULL; }
    return this->asan_shared;
  }

  void SaveLastCorpusInfo(bool used_corpus, size_t last_corpus_idx1, size_t last_corpus_idx2, size_t max_depth);
  size_t GetLastCorpusIndex1() { return this->basic_shared->last_corpus_index1; }
  size_t GetLastCorpusIndex2() { return this->basic_shared->last_corpus_index2; }
  size_t GetLastMaxDepth() { return this->basic_shared->last_max_depth; }
  bool GetLastUsedCorpus() { return this->basic_shared->last_used_corpus; }
  void IterStart();
  float GetIterStart();
  float GetLifetimeStart();

  void SaveLastReplay(Vector<RandSnapshot>* replay);
  void LoadLastReplay(Vector<RandSnapshot>* dest);
};

}
}

#endif
