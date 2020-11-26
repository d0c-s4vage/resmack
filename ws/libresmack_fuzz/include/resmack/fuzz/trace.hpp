#ifndef RESMACK_FUZZ_TRACE_H
#define RESMACK_FUZZ_TRACE_H

#include <string>

#include "resmack/rand.hpp"
#include "resmack/types.hpp"
#include "resmack/fuzz/serialized.hpp"
#include "resmack/fuzz/tracee.hpp"
#include "resmack/fuzz/trace_target.hpp"

namespace resmack {
namespace fuzz {

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
