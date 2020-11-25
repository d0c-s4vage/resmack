#ifndef RESMACK_FUZZ_TRACE_H
#define RESMACK_FUZZ_TRACE_H

#include <string>

#include "resmack/types.hpp"
#include "resmack/rand.hpp"

namespace resmack {
namespace fuzz {

struct TraceeShared {
  size_t num_snapshot_state;
};

class Tracee {
  // mmap'd shared space for IPC communication
  void* shared;

 public:
  Tracee();
  ~Tracee();

  void SaveLastCorpusIndex(bool used_corpus, size_t last_corpus_idx);
  void SaveLastReplay(Vector<RandSnapshot>* mutated_replay);
};

class Tracer {
 private:
  Tracee tracee;
 public:
  Tracer();
  ~Tracer();

  void Trace(pid_t pid);
  void Continue();
};

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

}
}

#endif
