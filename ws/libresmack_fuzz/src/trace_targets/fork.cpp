#include <sys/ptrace.h>
#include <unistd.h>
#include <fcntl.h>

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
    int fd = open("/dev/null", O_WRONLY);
    dup2(fd, 1);
    dup2(fd, 2);
    close(fd);


    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    cb_(tracee);
    std::exit(0);
  }
  return res;
}

}
}
}
