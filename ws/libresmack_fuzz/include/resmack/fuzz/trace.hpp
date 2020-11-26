#ifndef RESMACK_FUZZ_TRACE_H
#define RESMACK_FUZZ_TRACE_H

#include <string>

#include "resmack/types.hpp"
#include "resmack/rand.hpp"
#include "resmack/fuzz/trace_target.hpp"
#include "resmack/fuzz/serialized.hpp"

namespace resmack {
namespace fuzz {

struct TraceeShared {
  size_t num_snapshot_state;
};

class Tracee {
  size_t shared_max_size;
  // mmap'd shared space for IPC communication
  void* shared;
  // we don't need semaphores to guard these since they are ony ever written to
  // when the tracee is running, and only read when the tracee is paused
  size_t* shared_last_corpus_index;
  // boolean, uint32_t to help remember to align on 4-byte boundaries
  uint32_t* shared_last_used_corpus; 
  ser::GenStateHeader* shared_last_gen_state;

 public:
  Tracee();
  ~Tracee();

  void SaveLastCorpusIndex(bool used_corpus, size_t last_corpus_idx);
  size_t GetLastCorpusIndex() { return *this->shared_last_corpus_index; }
  bool GetLastUsedCorpus() { return (bool)*this->shared_last_used_corpus; }

  void SaveLastReplay(Vector<RandSnapshot>* replay);
  void LoadLastReplay(Vector<RandSnapshot>* dest);
};

class Tracer {
 private:
  Tracee tracee;
 public:
  Tracer();
  ~Tracer();

  // target does whatever it needs to do to return a pid_t and set whether
  // it should be attached or if it's good to go. The target is run in a
  // separate thread
  void Trace(TraceTarget* target);
};

/*
struct Registers {
  size_t rip;
  size_t rsp;
  size_t rbp;
  size_t rax;
  size_t rbx;
  size_t rcx;
  size_t rdx;
  size_t rsi;
  size_t rdi;
  size_t r8;
  size_t r9;
  size_t r10;
  size_t r11;
  size_t r12;
  size_t r13;
  size_t r14;
  size_t r15;
};

class Crash {
 public:
  Registers registers;
  int signal;
  size_t hash_major;
  size_t hash_minor;
  Vector<std::string> backtrace;

  static void TraceMe();

  Crash(Tracer* tracer);
  ~Crash();
};
*/

}
}

#endif
