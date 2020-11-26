#include <sys/ptrace.h>
#include <unistd.h>

#include "resmack/fuzz/tracee.hpp"
#include "resmack/fuzz/trace_targets/fork.hpp"

namespace resmack {
namespace fuzz {
namespace trace_targets {

Fork::Fork(TraceSpawnCb cb) : cb_(cb) {}
Fork::~Fork() {}

pid_t Fork::Spawn(Tracee* tracee) {
  pid_t res;
  if ((res = fork()) == 0) {
    cb_(tracee);
    std::exit(0);
  }
  return res;
}

}
}
}
