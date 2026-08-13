#include <cstring>
#include <cxxabi.h>
#include <format>
#include <iostream>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <libunwind.h>
#include <unistd.h>

#include "resmack/rand.hpp"
#include "resmack/debug.hpp"
#include "resmack/fuzz/serialized.hpp"
#include "resmack/fuzz/tracer.hpp"
#include "resmack/fuzz/utils.hpp"

namespace resmack {
namespace fuzz {

  // ----------------------------------------------------------------------------
  // TRACEE ---------------------------------------------------------------------
  // ----------------------------------------------------------------------------

  Tracee::Tracee(uint32_t idx): idx_(idx) {
    this->basic_shared_ = static_cast<TraceeShared*>(mmap(
      NULL,
      sizeof(*this->basic_shared_),
      PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS,
      -1,
      0
    ));
    if (this->basic_shared_ == MAP_FAILED) {
      throw std::runtime_error("Could not create basic_shared mmap: " + std::string(strerror(errno)));
    }

    asan_shared_ = static_cast<ser::AsanInfo*>(mmap(
      NULL,
      sizeof(*asan_shared_),
      PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS,
      -1,
      0
    ));
    if (asan_shared_ == MAP_FAILED) {
      throw std::runtime_error("Could not create asan_shared mmap: " + std::string(strerror(errno)));
    }

    this->Reset();
  }

  Tracee::~Tracee() {
    munmap(this->basic_shared_, sizeof(*this->basic_shared_));
    this->basic_shared_ = NULL;
    munmap(asan_shared_, sizeof(*asan_shared_));
    asan_shared_ = NULL;
  }

  void Tracee::SaveLastCorpusInfo(
    bool used_corpus,
    size_t last_corpus_idx1,
    size_t last_corpus_idx2,
    size_t max_depth
  ) {
    this->basic_shared_->last_used_corpus_ = (uint32_t)used_corpus;
    this->basic_shared_->last_corpus_index1_ = last_corpus_idx1;
    this->basic_shared_->last_corpus_index2_ = last_corpus_idx2;
    this->basic_shared_->last_max_depth_ = max_depth;
  }

  void Tracee::SaveLastReplay(Vector<RandSnapshot>* replay) {
    basic_shared_->last_gen_state_.num_states = replay->size();
    ser::GenState* curr = basic_shared_->states_;

    if (replay->size() >= TRACEE_MAX_LAST_GEN_STATES) {
      throw std::runtime_error(std::format(
        "Could not save the last replay: didn't expect this many states ({} > {})",
        replay->size(),
        TRACEE_MAX_LAST_GEN_STATES
      ));
    }

    for (RandSnapshot& state: *replay) {
      curr->ref_depth = state.ref_depth;
      curr->max_depth = state.max_depth;
      curr->rule_idx = state.rule_idx;
      memcpy(curr->rand_state, state.state, sizeof(state.state));
      curr++;
    }
  }

  void Tracee::LoadLastReplay(Vector<RandSnapshot>* dest) {
    size_t num_states = basic_shared_->last_gen_state_.num_states;

    for (size_t i = 0; i < num_states; i++) {
      const ser::GenState* curr = &basic_shared_->states_[i];
      dest->emplace_back(
        curr->ref_depth,
        curr->max_depth,
        curr->rule_idx,
        curr->rand_state
      );
    }
  }

  void Tracee::SaveAsanInfo(const char* report) {
    asan_shared_->exists = true;
    size_t report_size = strlen(report);
    size_t size_to_copy = report_size < sizeof(asan_shared_->report) ? report_size : sizeof(asan_shared_->report)-1;
    size_to_copy--; // null byte room
    memcpy(asan_shared_->report, report, size_to_copy);
    asan_shared_->report[size_to_copy+1] = 0;

    std::string major_stack;
    std::string minor_stack;

    unw_cursor_t cursor;
    unw_context_t context;

    // grab the machine context and initialize the cursor
    if (unw_getcontext(&context) < 0) {
      std::cout << "ERROR: cannot get local machine state" << std::endl;
      std::exit(1);
    }
    if (unw_init_local(&cursor, &context) < 0) {
      std::cout << "ERROR: cannot initialize cursor for local unwinding" << std::endl;
      std::exit(1);
    }

    char sym[4096];

    // currently the IP is within backtrace() itself so this loop
    // deliberately skips the first frame.
    size_t count = 0;
    while (unw_step(&cursor) > 0) {
      unw_word_t offset, pc;
      if (unw_get_reg(&cursor, UNW_REG_IP, &pc)) {
        throw std::runtime_error("Could not get ip from unwind");
      }

      if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
        int status;
        size_t demangled_size;
        char* demangled = abi::__cxa_demangle(sym, NULL, &demangled_size, &status);
        if (demangled != NULL) {
          snprintf(sym, sizeof(sym), "%s+0x%lx", demangled, offset);
          free(demangled);
        } else {
          snprintf(sym + strlen(sym), sizeof(sym) - strlen(sym), "+0x%lx", offset);
        }
      } else {
        snprintf(sym, sizeof(sym), "??");
      }

      if (count <= 5) {
        if (count > 0) { major_stack += "\n"; }
        major_stack += sym;
      }
      if (count > 0) { minor_stack += "\n"; }
      minor_stack += sym;
    }

    utils::sha1_hex(major_stack.data(), major_stack.size(), asan_shared_->major_hash);
    utils::sha1_hex(minor_stack.data(), minor_stack.size(), asan_shared_->minor_hash);
  }

  void Tracee::IterStart() {
    basic_shared_->iter_start_ = utils::GetTimeNow();
  }

  float Tracee::GetIterStart() {
    return basic_shared_->iter_start_;
  }

  float Tracee::GetLifetimeStart() {
    return basic_shared_->lifetime_start_;
  }

  void Tracee::Reset() {
    asan_shared_->exists = false;
    basic_shared_->iter_start_ = -1.0f;
    basic_shared_->lifetime_start_ = utils::GetTimeNow();
  }

}
}
