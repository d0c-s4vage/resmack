#include "errno.h"
#include <iostream>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#include "sys/stat.h"

#include "resmack/fuzz/states/mmap.hpp"
#include "resmack/fuzz/ipc_util.hpp"
#include "resmack/fuzz/corpus.hpp"
#include "resmack/fuzz/corpora/mmap.hpp"

namespace resmack {
namespace fuzz {
namespace states {

MmapState::MmapState(const char* state_path) : state_path(state_path) {
  this->state_max_size = 0x400 * 0x400 * 100; // 100 MB
  struct stat info;
  bool is_new = false;

  if (stat(this->state_path, &info) == 0) {
    this->state_file = fopen(this->state_path, "r+b");
  } else {
    this->state_file = fopen(this->state_path, "w+b");
    ftruncate(fileno(this->state_file), this->state_max_size);
    is_new = true;
  }

  this->state_map = mmap(
    NULL,
    state_max_size,
    PROT_READ | PROT_WRITE,
    MAP_SHARED_VALIDATE, // | MAP_ANONYMOUS // with no file?
    fileno(this->state_file), // FD
    0   // offset
  );

  if (this->state_map == MAP_FAILED) {
    perror("Could not create state map");
    std::exit(1);
  }

  this->metadata = (StateMetadata*)this->state_map;
  if (is_new) {
    this->InitNewStats();
  }

  size_t meta_size = sizeof(StateMetadata);
  this->corpus.Init((void*)((char*)this->state_map + meta_size), state_max_size - meta_size);

  //if ((this->state_lock = sem_open(this->state_path, O_CREAT, 0660, 1)) == SEM_FAILED) {
  if ((this->state_lock = sem_open("/ttest12345", O_CREAT, 0660, 1)) == SEM_FAILED) {
    perror("Could not create semaphore");
    std::exit(1);
  }
}

MmapState::~MmapState() {
  munmap(this->state_map, this->state_max_size);
  fclose(this->state_file);
  sem_close(this->state_lock);
}

void MmapState::InitNewStats() {
#define STAT(NAME) this->metadata->stats.duration_##NAME = 0;
#include "resmack/fuzz/stats.def"
#undef STAT
}

void MmapState::SyncStats(TargetStats* stats) {
  if (sem_wait(this->state_lock) == -1) {
    perror("SyncStats (sem_wait)");
    std::exit(1);
  }

#define STAT(NAME) \
  this->metadata->stats.duration_##NAME += stats->duration_##NAME;
#include "resmack/fuzz/stats.def"
#undef STAT

  if (sem_post(this->state_lock) == -1) {
    perror("SyncStats (sem_post)");
    std::exit(1);
  }
}

size_t MmapState::GetNumIterations() {
  return this->metadata->iterations;
}
void MmapState::IncNumIterations() {
  this->IncNumIterations(1);
}
void MmapState::IncNumIterations(uint64_t amt) {
  WITH_LOCK(this->state_lock, IncNumIterations, { 
    this->metadata->iterations += amt;
  });
}

size_t MmapState::GetNumCrashes() {
  return this->metadata->crashes;
}
void MmapState::IncNumCrashes() {
  this->IncNumCrashes(1);
}
void MmapState::IncNumCrashes(uint64_t amt) {
  WITH_LOCK(this->state_lock, IncNumCrashes, {
    this->metadata->crashes += amt;
  });
}

Corpus* MmapState::GetCorpus() {
  return &this->corpus;
}

}
}
}
