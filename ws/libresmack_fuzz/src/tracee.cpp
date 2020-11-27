#include <cstring>
#include <iostream>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "resmack/rand.hpp"
#include "resmack/fuzz/serialized.hpp"
#include "resmack/fuzz/trace.hpp"

namespace resmack {
namespace fuzz {

// ----------------------------------------------------------------------------
// TRACEE ---------------------------------------------------------------------
// ----------------------------------------------------------------------------

Tracee::Tracee() {
  this->shared_max_size = 0x10000;
  this->shared = mmap(
    NULL,
    0x100000,
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
}

Tracee::~Tracee() {
  munmap(this->shared, this->shared_max_size);
  this->shared_last_corpus_index = NULL;
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
    dest->emplace_back(curr->ref_depth, curr->rule_idx, curr->rand_state);
  }
}

}
}
