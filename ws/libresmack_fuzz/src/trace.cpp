#include "sys/ptrace.h"
#include "sys/types.h"
#include "sys/wait.h"

#include "resmack/fuzz/trace.hpp"

namespace resmack {
namespace fuzz {

Tracer::Tracer() {}
Tracer::~Tracer() {}

void Tracer::TraceMe() {
  ptrace(PTRACE_TRACEME, 0, NULL, NULL);
}

}
}
