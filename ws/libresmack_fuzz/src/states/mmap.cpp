#include "errno.h"
#include <iostream>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#include "sys/stat.h"

#include "resmack/fuzz/states/mmap.hpp"

namespace resmack {
namespace fuzz {
namespace states {

MmapState::MmapState(const char* state_path) : state_path(state_path) {
  this->state_max_size = 0x400 * 0x400 * 100; // 100 MB
  struct stat info;

  if (stat(this->state_path, &info) == 0) {
    this->state_file = fopen(this->state_path, "r+b");
  } else {
    this->state_file = fopen(this->state_path, "w+b");
    ftruncate(fileno(this->state_file), this->state_max_size);
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

}
}
}
