#ifndef RESMACK_FUZZ_TRACEE_H
#define RESMACK_FUZZ_TRACEE_H

#include <inttypes.h>

#include "resmack/rand.hpp"
#include "resmack/fuzz/serialized.hpp"

namespace resmack {
namespace fuzz {

struct TraceeShared {
  size_t num_snapshot_state;
};

class Tracee {
  size_t shared_max_size;
  // mmap'd shared space for IPC communication
  void* shared;

  // we don't need semaphores to guard these since they are ony ever written to
  // when the tracee is running, and only read when the tracee is paused
  size_t* shared_last_corpus_index;
  size_t* shared_last_max_depth;
  // boolean, uint32_t to help remember to align on 4-byte boundaries
  uint32_t* shared_last_used_corpus; 
  ser::GenStateHeader* shared_last_gen_state;

  // mmap'd shared space to relay asan report information
  size_t asan_shared_max_size;
  void* asan_shared;
  ser::AsanInfo* asan_info;

 public:
  Tracee();
  ~Tracee();

  void Reset();

  void SaveAsanInfo(const char* report);
  ser::AsanInfo* GetAsanInfo() {
    if (!this->asan_info->exists) { return NULL; }
    return this->asan_info;
  }

  void SaveLastCorpusInfo(bool used_corpus, size_t last_corpus_idx, size_t max_depth);
  size_t GetLastCorpusIndex() { return *this->shared_last_corpus_index; }
  size_t GetLastMaxDepth() { return *this->shared_last_max_depth; }
  bool GetLastUsedCorpus() { return (bool)*this->shared_last_used_corpus; }

  void SaveLastReplay(Vector<RandSnapshot>* replay);
  void LoadLastReplay(Vector<RandSnapshot>* dest);
};

}
}

#endif
