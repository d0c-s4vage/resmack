#include <errno.h>
#include <cstddef>
#include <iostream>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "resmack/fuzz/states/mmap.hpp"
#include "resmack/fuzz/ipc_util.hpp"
#include "resmack/fuzz/corpus.hpp"
#include "resmack/fuzz/corpora/mmap.hpp"
#include "resmack/fuzz/utils.hpp"

namespace resmack {
namespace fuzz {
namespace states {

  MmapState::MmapState(const char* state_path) : state_path(state_path), state_lock(state_path) {
    this->state_max_size = static_cast<size_t>(0x400 * 0x400 * 200); // 100 MB
    struct stat info;
    bool is_new = false;

    if (stat(this->state_path, &info) == 0) {
      this->state_file = fopen(this->state_path, "r+b");
      if (this->state_file == NULL) {
        perror("Could not create new state file");
        std::exit(1);
      }
    } else {
      this->state_file = fopen(this->state_path, "w+b");
      if (this->state_file == NULL) {
        perror("Could not open existing state file");
        std::exit(1);
      }
      if (ftruncate(fileno(this->state_file), this->state_max_size) != 0) {
        perror("Could not create resmack state mmap");
        std::exit(1);
      }
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
    this->corpus.Init(
      this->state_path,
      (void*)((char*)this->state_map + meta_size),
      state_max_size - meta_size
    );
    this->corpus.SetCurrIterPtr(&this->metadata->iterations);
  }

  MmapState::~MmapState() {
    munmap(this->state_map, this->state_max_size);
    fclose(this->state_file);
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

  size_t MmapState::GetNumIterations() {
    return this->metadata->iterations;
  }
  void MmapState::IncNumIterations() {
    this->IncNumIterations(1);
  }
  void MmapState::IncNumIterations(uint64_t amt) {
    {
      std::scoped_lock _lock(this->state_lock);
      this->metadata->iterations += amt;
    }
  }

  size_t MmapState::GetNumCrashes() {
    return this->metadata->crashes;
  }
  void MmapState::IncNumCrashes() {
    this->IncNumCrashes(1);
  }
  void MmapState::IncNumCrashes(uint64_t amt) {
    DEBUG_PRINT("INCREMENTING CRASHES\n");
    {
      std::scoped_lock _lock(this->state_lock);
      this->metadata->crashes += amt;
    }
  }
  void MmapState::IncNumCrashesIfTrue(UniqueCrashCb cb) {
    {
      std::scoped_lock _lock(this->state_lock);
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
