#include "errno.h"
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
#include "sys/stat.h"

#include "resmack/fuzz/states/mmap.hpp"

namespace resmack {
namespace fuzz {
namespace states {

MmapState::MmapState(const char* state_path) {
  this->state_max_size = 0x400 * 0x400 * 100; // 100 MB
  struct stat info;

  if (stat(state_path, &info) == 0) {
    this->state_file = fopen(state_path, "r+b");
  } else {
    this->state_file = fopen(state_path, "w+b");
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
    perror("Could not create state map!");
    std::exit(1);
  }

  this->metadata = (StateMetadata*)this->state_map;
}

MmapState::~MmapState() {
  munmap(this->state_map, this->state_max_size);
  fclose(this->state_file);
}

size_t MmapState::GetNumIterations() {
  return this->metadata->iterations;
}
void MmapState::IncNumIterations() {
  this->metadata->iterations++;
}

size_t MmapState::GetNumCrashes() {
  return this->metadata->crashes;
}
void MmapState::IncNumCrashes() {
  this->metadata->crashes++;
}

}
}
}
