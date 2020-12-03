#include <iostream>
#include <sys/ptrace.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include "resmack/fuzz/tracee.hpp"
#include "resmack/fuzz/trace_targets/fork.hpp"

#include "asan_util.hpp"

namespace resmack {
namespace fuzz {
namespace trace_targets {

Fork::Fork(TraceSpawnCb cb) : cb_(cb) {}
Fork::~Fork() {}

pid_t Fork::Spawn(Tracee* tracee) {
  pid_t res;
  if ((res = fork()) == 0) {
    /*
    int fd = open("/dev/null", O_WRONLY);
    dup2(fd, 1);
    dup2(fd, 2);
    close(fd);
    */

    resmack::fuzz::asan::SetAsanCallback([tracee](const char* report) {
      tracee->SaveAsanInfo(report);
      // let it die, the tracer knows to look for the ASAN_EXIT_CODE
    });

    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    cb_(tracee);
    std::exit(0);
  }
  return res;
}

}
}
}
