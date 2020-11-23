#ifndef RESMACK_FUZZ_STATE_H
#define RESMACK_FUZZ_STATE_H

#include "fcntl.h"
#include "inttypes.h"
#include "stdio.h"
#include "sys/mman.h"
#include "sys/stat.h"
#include "semaphore.h"
#include "unistd.h"

#include "resmack/fuzz/state.hpp"
#include "resmack/fuzz/corpus.hpp"
#include <bits/stdint-uintn.h>

#define WITH_LOCK(LOCK, MSG, STATEMENTS) { \
  if (sem_wait(LOCK) == -1) { \
    perror("MSG (sem_wait)"); \
    std::exit(1); \
  } \
  STATEMENTS\
  if (sem_post(LOCK) == -1) { \
    perror("MSG (sem_post)"); \
    std::exit(1); \
  } \
}

namespace resmack {
namespace fuzz {
namespace states {

struct __attribute__ ((packed)) StateMetadata {
  uint64_t iterations;
  uint64_t crashes;
  uint64_t reserved1;
  uint64_t reserved2;
  uint64_t reserved3;
  uint64_t reserved4;
  uint64_t reserved5;
  uint64_t reserved6;
  uint64_t reserved7;
  uint64_t reserved8;
  uint64_t reserved9;
  uint64_t reserved10;
  uint64_t reserved11;
  uint64_t reserved12;
  uint64_t reserved13;
  uint64_t reserved14;
};

class MmapState {
 private:
  const char* state_path;
  FILE* state_file;
  size_t state_max_size;

  sem_t* state_lock;

  void* state_map;
  StateMetadata* metadata;

 public:
  MmapState(const char* statePath);
  ~MmapState();

  uint64_t GetNumIterations();
  void IncNumIterations();
  void IncNumIterations(uint64_t amt);

  uint64_t GetNumCrashes();
  void IncNumCrashes();
  void IncNumCrashes(uint64_t amt);

  Corpus* GetCorpus();
};

}
}
}

#endif
