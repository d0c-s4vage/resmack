#ifndef RESMACK_FUZZ_MMAP_STATE_H
#define RESMACK_FUZZ_MMAP_STATE_H

#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <bits/stdint-uintn.h>

#include "resmack/fuzz/state.hpp"
#include "resmack/fuzz/target.hpp"
#include "resmack/fuzz/state.hpp"
#include "resmack/fuzz/corpus.hpp"
#include "resmack/fuzz/corpora/mmap.hpp"

namespace resmack {
namespace fuzz {
namespace states {

struct StateMetadata {
  uint64_t iterations;
  uint64_t crashes;
  StateStats stats;
  uint64_t reserved1;
  uint64_t reserved2;
  uint64_t reserved3;
  uint64_t reserved4;
  uint64_t reserved5;
  uint64_t reserved6;
  uint64_t reserved7;
  uint64_t reserved8;
};

class MmapState : public State {
 private:
  const char* state_path;
  FILE* state_file;
  size_t state_max_size;

  sem_t* state_lock;

  void* state_map;
  StateMetadata* metadata;

  corpora::MmapCorpus corpus;

 public:
  MmapState(const char* statePath);
  ~MmapState();

  StateStats* GetStats() { return &this->metadata->stats; };
  void InitNewStats();
  //void SyncStats(targets::TargetStats* stats);

  uint64_t GetNumIterations();
  void IncNumIterations();
  void IncNumIterations(uint64_t amt);

  uint64_t GetNumCrashes();
  void IncNumCrashesIfTrue(UniqueCrashCb cb);
  void IncNumCrashes();
  void IncNumCrashes(uint64_t amt);

  Corpus* GetCorpus();
  corpora::MmapCorpus* GetMmapCorpus();

  void SyncLockAcquire();
  void SyncLockRelease();
};

}
}
}

#endif
