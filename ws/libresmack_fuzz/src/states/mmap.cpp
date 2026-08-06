#include <errno.h>
#include <filesystem>
#include <cstddef>
#include <iostream>
#include <semaphore.h>
#include <mutex>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "resmack/fuzz/states/mmap.hpp"
#include "resmack/fuzz/corpus.hpp"
#include "resmack/fuzz/corpora/mmap.hpp"
#include "resmack/fuzz/utils.hpp"

namespace fs = std::filesystem;

#ifndef RESMACK_MAX_STATE_SIZE_MB
#define RESMACK_MAX_STATE_SIZE_MB 200
#endif

namespace resmack {
namespace fuzz {
namespace states {

  MmapState::MmapState(fs::path state_path) :
    state_path(state_path),
    state_lock(state_path),
    corpus(state_path)
  {
    this->state_max_size = static_cast<size_t>(0x400 * 0x400 * RESMACK_MAX_STATE_SIZE_MB);
    bool is_new = !fs::exists(this->state_path);
    const char* open_flags = is_new ? "w+b" : "r+b";

    this->state_file = std::fopen(this->state_path.c_str(), open_flags);
    if (this->state_file == NULL) {
      utils::throw_runtime_error(std::format("Could not open state file at {}", this->state_path.string()));
    }

    struct stat st;
    fstat(fileno(this->state_file), &st);
    bool should_init = is_new || (size_t)st.st_size != this->state_max_size;

    if (ftruncate(fileno(this->state_file), this->state_max_size) != 0) {
      utils::throw_runtime_error(std::format("Could not initialize state size for {}", this->state_path.string()));
    }

    this->state_map = mmap(
      NULL,
      this->state_max_size,
      PROT_READ | PROT_WRITE,
      MAP_SHARED_VALIDATE, // | MAP_ANONYMOUS // with no file?
      fileno(this->state_file), // FD
      0   // offset
    );
    if (this->state_map == MAP_FAILED) {
      utils::throw_runtime_error("Could not create state map");
    }

    madvise(this->state_map, this->state_max_size, MADV_RANDOM);

    this->metadata = (StateMetadata*)this->state_map;
    if (should_init) {
      this->InitNewStats();
    }

    size_t meta_size = sizeof(StateMetadata);
    this->corpus.Init(
      (void*)((char*)this->state_map + meta_size),
      state_max_size - meta_size
    );
    this->corpus.SetCurrIterPtr(&this->metadata->iterations);
  }

  MmapState::~MmapState() {
    munmap(this->state_map, this->state_max_size);
    std::fclose(this->state_file);
  }

  void MmapState::InitNewStats() {
#define STAT(NAME) this->metadata->stats.duration_##NAME = 0;
#include "resmack/fuzz/stats.def"
#undef STAT
  }

  void MmapState::SyncStats(TargetStats* stats) {
    this->corpus.SyncCounters();

    {
      std::scoped_lock _l(this->state_lock);

#define STAT(NAME) \
      this->metadata->stats.duration_##NAME += stats->duration_##NAME;
#include "resmack/fuzz/stats.def"
#undef STAT
    }
  }

  uint64_t MmapState::GetNumIterations() {
    return this->metadata->iterations;
  }
  void MmapState::IncNumIterations() {
    this->IncNumIterations(1);
  }
  void MmapState::IncNumIterations(uint64_t amt) {
    this->metadata->iterations += amt;
  }

  uint64_t MmapState::GetNumCrashes() {
    return this->metadata->crashes;
  }
  void MmapState::IncNumCrashes() {
    this->IncNumCrashes(1);
  }
  void MmapState::IncNumCrashes(uint64_t amt) {
      this->metadata->crashes += amt;
  }
  void MmapState::IncNumCrashesIfTrue(UniqueCrashCb cb) {
    {
      if (cb()) {
        this->metadata->crashes += 1;
      }
    }
  }

  Corpus* MmapState::GetCorpus() {
    return &this->corpus;
  }

  corpora::MmapCorpus* MmapState::GetMmapCorpus() {
    return &this->corpus;
  }

}
}
}
