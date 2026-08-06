#include <mm_malloc.h>
#include <cstring>
#include <fcntl.h>
#include <stdio.h>
#include <semaphore.h>
#include <mutex>
#include <sys/mman.h>

#include "xxhash.h"

#include "resmack/fuzz/feedbacks/coverage.hpp"

namespace resmack {
namespace fuzz {
  void HandleSanitizerCovTracePcGuardInit(uint32_t* start, uint32_t* stop) noexcept {
    if (start == stop || *start) return;  // Initialize only once.
    for (uint32_t *x = start; x < stop; x++) {
      *x = ++NUM_COV_FLAGS;  // Guards should start from 1.
    }
  }

  void HandleSanitizerCovTracePcGuard(uint32_t* guard) noexcept {
    if (nullptr == guard || !*guard || nullptr == FPO_COV_DATA) { return; }

    uint32_t idx = *guard - 1;
    uint8_t old = FPO_COV_DATA->flags[idx];
    FPO_COV_DATA->flags[idx] = 1;
    FPO_COV_DATA->fpo_saw_new |= (old == 0);
    FPO_COV_DATA->hit_flags += (old == 0);
  }

  /*
  void _sanitizer_print_guard_source(uint32_t* guard) {
       void *pc = __builtin_return_address(0);
       char pc_desc[1024];
       __sanitizer_symbolize_pc(pc, "%p %F %L", pc_desc, sizeof(pc_desc));
       printf("guard: %p %x PC %s\n", guard, *guard, pc_desc);
  }
  */



  // --------------------------------------------------------------------------

  /**
   * This class defines coverage-based feedback - the fuzzing process must be compiled
   * with -fsanitize-coverage=trace-pc-guard
   *
   * This class assumes that it will be shared between processes: parent proc <-> fuzz proc.
   *
   * - Coverage flags are updated via the HandleSanitizerCovTracePcGuard function
   * - Syncing only occurs when the local hash value is different than the shared hash value
   * - The fuzzing process crashes - the tracer will call Sync() on the feedback instance.
   */
  Coverage::Coverage() :
    sync_lock_("CovSyncLock", true),
    cov_data_size_(
      sizeof(CoverageData) +
      sizeof(CoverageData::flags[0]) * MAX_COV_FLAGS
    ) {
    void* map_result = mmap(
      nullptr,
      cov_data_size_,
      PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS,
      -1,
      0
    );
    if (map_result == MAP_FAILED) {
      throw std::runtime_error("Could not create coverage mmap: " + std::string(std::strerror(errno)));
    }

    shared_data_ = std::unique_ptr<CoverageData, MMapDeleter>(static_cast<CoverageData*>(map_result), MMapDeleter(cov_data_size_));
    fpo_data_ = std::unique_ptr<CoverageData, AlignedDeleter>(static_cast<CoverageData*>(_mm_malloc(cov_data_size_, 64)));

    memset(fpo_data_.get(), 0, cov_data_size_);
    memset(shared_data_.get(), 0, cov_data_size_);

    shared_data_->num_flags = NUM_COV_FLAGS;
    fpo_data_->num_flags = NUM_COV_FLAGS;

    FPO_COV_DATA = fpo_data_.get();
  }

  Coverage::~Coverage() {
    FPO_COV_DATA = nullptr;
  }

  std::string Coverage::GetSummary() const noexcept {
    return std::to_string(fpo_data_->hit_flags) + " edges";
  }

  void Coverage::FPOStart() noexcept {
    fpo_data_->fpo_saw_new = false;
    FPO_COV_DATA = fpo_data_.get();
  }

  void Coverage::FPOStop() noexcept {
    FPO_COV_DATA = nullptr;

    // fpo_saw_new == true when we saw a new edge locally
    // shared_data_->hash won't equal fpo_data_->hash when new coverage was synced
    // by someone else to shared_data_
    if (fpo_data_->fpo_saw_new || shared_data_->hash != fpo_data_->hash) {
      Sync();
    }
  }

  uint64_t Coverage::CalcHash() {
    // Allocate a contiguous memory buffer to safely read into
    static std::vector<uint8_t> buffer(fpo_data_->num_flags);
    for (size_t i = 0; i < fpo_data_->num_flags; ++i) {
        buffer[i] = fpo_data_->flags[i];
    }

    // Pass the contiguous raw byte buffer to XXH3
    return XXH3_64bits(buffer.data(), buffer.size());
  }

  bool Coverage::Sync() noexcept {
    // only one process should be syncing with the shared_data_ at a time
    std::scoped_lock _l(sync_lock_);

    // used to track if we *ACTUALLY* had new coverage (after comparing to the shared data).
    // fpo_data_->fpo_has_new is if we observed a new edge locally (but another fuzzing process
    // may have found it and already synced it)
    //
    // (NOTE: shared_data_->fpo_has_new isn't used)
    bool we_had_new = false;
    bool they_had_new = false;

    for (size_t i = 0; i < fpo_data_->num_flags; i++) {
        uint64_t ours   = fpo_data_->flags[i];
        uint64_t shared = shared_data_->flags[i];

        uint64_t merged = shared | ours;
        we_had_new |= ((ours & ~shared) != 0);
        they_had_new |= ((shared & ~ours) != 0);

        fpo_data_->flags[i] = merged;
        shared_data_->flags[i] = merged;
    }

    if (we_had_new || they_had_new) {
      fpo_data_->hash = CalcHash();
      shared_data_->hash = fpo_data_->hash;
    }

    return fpo_data_->fpo_saw_new;
  }

  FeedbackStats Coverage::GetStats() const noexcept {
    return {
      .new_coverage = fpo_data_->fpo_saw_new,
      .key = fpo_data_->hash,
      .num = fpo_data_->hit_flags,
    };
  }

}
}
