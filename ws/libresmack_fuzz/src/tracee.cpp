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

  Tracee::Tracee(uint32_t idx): idx(idx) {
    this->basic_shared = static_cast<TraceeShared*>(mmap(
      NULL,
      sizeof(*this->basic_shared),
      PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS,
      -1,
      0
    ));
    if (this->basic_shared == MAP_FAILED) {
      throw std::runtime_error("Could not create basic_shared mmap: " + std::string(strerror(errno)));
    }


    this->asan_shared = static_cast<ser::AsanInfo*>(mmap(
      NULL,
      sizeof(*this->asan_shared),
      PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS,
      -1,
      0
    ));
    if (this->asan_shared == MAP_FAILED) {
      throw std::runtime_error("Could not create asan_shared mmap: " + std::string(strerror(errno)));
    }

    this->Reset();
  }

  Tracee::~Tracee() {
    munmap(this->basic_shared, sizeof(*this->basic_shared));
    this->basic_shared = NULL;
    munmap(this->asan_shared, sizeof(*this->asan_shared));
    this->asan_shared = NULL;
  }

  void Tracee::SaveLastCorpusInfo(
    bool used_corpus,
    size_t last_corpus_idx1,
    size_t last_corpus_idx2,
    size_t max_depth
  ) {
    this->basic_shared->last_used_corpus = (uint32_t)used_corpus;
    this->basic_shared->last_corpus_index1 = last_corpus_idx1;
    this->basic_shared->last_corpus_index2 = last_corpus_idx2;
    this->basic_shared->last_max_depth = max_depth;
  }

  void Tracee::SaveLastReplay(Vector<RandSnapshot>* replay) {
    this->basic_shared->last_gen_state.num_states = replay->size();
    ser::GenState* curr = this->basic_shared->states;

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
    size_t num_states = this->basic_shared->last_gen_state.num_states;

    for (size_t i = 0; i < num_states; i++) {
      const ser::GenState* curr = &this->basic_shared->states[i];
      dest->emplace_back(
        curr->ref_depth,
        curr->max_depth,
        curr->rule_idx,
        curr->rand_state
      );
    }
  }

  void Tracee::SaveAsanInfo(const char* report) {
    this->asan_shared->exists = true;
    size_t report_size = strlen(report);
    size_t size_to_copy = report_size < sizeof(this->asan_shared->report) ? report_size : sizeof(this->asan_shared->report)-1;
    size_to_copy--; // null byte room
    memcpy(this->asan_shared->report, report, size_to_copy);
    this->asan_shared->report[size_to_copy+1] = 0;

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

    utils::sha1_hex(major_stack.data(), major_stack.size(), this->asan_shared->major_hash);
    utils::sha1_hex(minor_stack.data(), minor_stack.size(), this->asan_shared->minor_hash);
  }

  void Tracee::IterStart() {
    this->basic_shared->iter_start = utils::GetTimeNow();
  }

  float Tracee::GetIterStart() {
    return this->basic_shared->iter_start;
  }

  float Tracee::GetLifetimeStart() {
    return this->basic_shared->lifetime_start;
  }

  void Tracee::Reset() {
    this->asan_shared->exists = false;
    this->basic_shared->iter_start = -1.0f;
    this->basic_shared->lifetime_start = utils::GetTimeNow();
  }

}
}
