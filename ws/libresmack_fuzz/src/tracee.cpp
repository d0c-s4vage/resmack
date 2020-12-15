#include <cstring>
#include <cxxabi.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <libunwind.h>

#include "resmack/rand.hpp"
#include "resmack/fuzz/serialized.hpp"
#include "resmack/fuzz/trace.hpp"
#include "resmack/fuzz/utils.hpp"

namespace resmack {
namespace fuzz {

// ----------------------------------------------------------------------------
// TRACEE ---------------------------------------------------------------------
// ----------------------------------------------------------------------------

Tracee::Tracee() {
  this->shared_max_size = 0x10000;
  this->shared = mmap(
    NULL,
    this->shared_max_size,
    PROT_READ | PROT_WRITE,
    MAP_SHARED | MAP_ANONYMOUS,
    -1,
    0
  );

  if (this->shared == MAP_FAILED) {
    perror("Could not create tracee mmap");
    std::exit(1);
  }

  this->shared_last_used_corpus = (uint32_t*)this->shared;
  this->shared_last_corpus_index =
    (size_t*)(this->shared_last_used_corpus + sizeof(uint32_t));
  this->shared_last_max_depth =
    (size_t*)(this->shared_last_corpus_index + sizeof(size_t));

  this->shared_last_gen_state =
    (ser::GenStateHeader*)(this->shared_last_max_depth + sizeof(size_t));

  this->asan_shared_max_size = sizeof(ser::AsanInfo);
  this->asan_shared = mmap(
    NULL,
    this->asan_shared_max_size,
    PROT_READ | PROT_WRITE,
    MAP_SHARED | MAP_ANONYMOUS,
    -1,
    0
  );
  if (this->asan_shared == MAP_FAILED) {
    perror("Could not create tracee asan mmap");
    std::exit(1);
  }

  this->asan_info = (ser::AsanInfo*)this->asan_shared;
  this->asan_info->exists = false;
}

Tracee::~Tracee() {
  munmap(this->shared, this->shared_max_size);
  munmap(this->asan_shared, this->asan_shared_max_size);
  this->shared_last_corpus_index = NULL;
  this->shared_last_max_depth = NULL;
  this->shared_last_gen_state = NULL;
}

void Tracee::SaveLastCorpusInfo(
  bool used_corpus,
  size_t last_corpus_idx,
  size_t max_depth
) {
  *this->shared_last_used_corpus = (uint32_t)used_corpus;
  *this->shared_last_corpus_index = last_corpus_idx;
  *this->shared_last_max_depth = max_depth;
}

void Tracee::SaveLastReplay(Vector<RandSnapshot>* replay) {
  this->shared_last_gen_state->num_states = replay->size();

  size_t curr_offset = sizeof(ser::GenStateHeader);
  ser::GenState* curr;

  for (RandSnapshot& state: *replay) {
    curr = (ser::GenState*)this->shared_last_gen_state + curr_offset;
    curr->ref_depth = state.ref_depth;
    curr->max_depth = state.max_depth;
    curr->rule_idx = state.rule_idx;
    memcpy(curr->rand_state, state.state, sizeof(state.state));
    curr_offset += sizeof(ser::GenState);
  }
}

void Tracee::LoadLastReplay(Vector<RandSnapshot>* dest) {
  size_t num_states = this->shared_last_gen_state->num_states;
  size_t curr_offset = sizeof(ser::GenStateHeader);
  ser::GenState* curr;

  for (; num_states > 0; num_states--) {
    curr = (ser::GenState*)this->shared_last_gen_state + curr_offset;
    dest->emplace_back(
      curr->ref_depth,
      curr->max_depth,
      curr->rule_idx,
      curr->rand_state
    );
    curr_offset += sizeof(ser::GenState);
  }
}

void Tracee::SaveAsanInfo(const char* report) {
  this->asan_info->exists = true;
  size_t report_size = strlen(report);
  size_t size_to_copy = report_size < this->asan_shared_max_size ? report_size: this->asan_shared_max_size;
  size_to_copy--; // null byte room
  memcpy(this->asan_info->report, report, size_to_copy);
  this->asan_info->report[size_to_copy+1] = 0;

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
      std::cout << "ERROR: Can't get ip" << std::endl;
      std::exit(1);
    }

    printf("0x%lx: ", pc);

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

  utils::sha1_hex(major_stack.data(), major_stack.size(), this->asan_info->major_hash);
  utils::sha1_hex(minor_stack.data(), minor_stack.size(), this->asan_info->minor_hash);
}

void Tracee::Reset() {
  this->asan_info->exists = false;
}

}
}
