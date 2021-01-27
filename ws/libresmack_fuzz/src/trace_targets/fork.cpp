#include <iostream>
#include <sys/ptrace.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include "resmack/fuzz/tracee.hpp"
#include "resmack/fuzz/trace_targets/fork.hpp"
#include "resmack/fuzz/utils.hpp"
#include "resmack/fuzz/ipc_util.hpp"

#include "asan_util.hpp"

namespace resmack {
namespace fuzz {
namespace trace_targets {

// install signal handler here to catch SIGINT --> set SHUTTING_DOWN = true

void sigint_handler(int signum) {
  resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK.lock();
  _exit(0); // no cleanup!
}

Fork::Fork(bool mute_io, TraceSpawnCb cb) : cb(cb), mute_io(mute_io) {}
Fork::~Fork() {}

pid_t Fork::Spawn(Tracee* tracee) {
  pid_t res;
  if ((res = fork()) == 0) {
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
      perror("  >Forkee: Could not install signal handler in forked process\n");
      std::cout << std::flush;
      std::cerr << std::flush;
    }

    if (this->mute_io) {
      int fd = open("/dev/null", O_WRONLY);
      dup2(fd, 1);
      dup2(fd, 2);
      close(fd);
    }

    signal(SIGINT, sigint_handler);

    resmack::fuzz::asan::SetAsanCallback([tracee](const char* report) {
      printf("ASAN REPORT!!!!\n\n%s\n", report);
      std::cout << std::flush;
      if (tracee == NULL) { return; }
      tracee->SaveAsanInfo(report);
      // let it die, the tracer knows to look for the ASAN_EXIT_CODE
    });

    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    this->cb(tracee);
    _exit(0);
  }

  std::cout << std::flush;
  return res;
}

}
}
}
