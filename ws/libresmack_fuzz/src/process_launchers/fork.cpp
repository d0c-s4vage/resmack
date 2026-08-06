#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <csignal>
#include <sys/ptrace.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "resmack/debug.hpp"

#include "resmack/fuzz/asan_util.hpp"
#include "resmack/fuzz/process_utils.hpp"
#include "resmack/fuzz/tracee.hpp"
#include "resmack/fuzz/utils.hpp"
#include "resmack/fuzz/process_launchers/fork.hpp"

namespace resmack {
namespace fuzz {
namespace process_launchers {
  ForkLauncher::ForkLauncher(bool mute_io, TraceSpawnCb cb) : cb(cb), mute_io(mute_io) {}
  ForkLauncher::~ForkLauncher() {}

  pid_t ForkLauncher::Spawn(Tracee* tracee) {
    tracee->Reset();

    DEBUG_PRINT("SPAWNING\n");
    pid_t fork_pid = fork();
    if (fork_pid == 0) {
      if (!process_utils::IgnoreBasicSignals()) {
        _exit(1);
      }

      DEBUG_PRINT("New forked child\n");

      if (mute_io) {
        DEBUG_PRINT("ForkLauncher child: Muting IO\n");
        int fd = open("/dev/null", O_WRONLY);
        fflush(stdout);
        dup2(fd, 1);
        fflush(stderr);
        dup2(fd, 2);
        close(fd);
      }

      resmack::fuzz::asan::SetAsanCallback([tracee](const char* report) {
        DEBUG_PRINT("ForkLauncher child: Handling ASAN report!\n");
        if (tracee == NULL) { return; }
        tracee->SaveAsanInfo(report);
      });

      if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
        throw std::runtime_error("Forked process could not call PTRACE_TRACEME: " + std::string(std::strerror(errno)));
      }

      DEBUG_PRINT("ForkLauncher child: Signalling to parent that we're ready (with a sigstop)\n");

      // this gives the parent process a chance to catch up. It will tell
      // us to continue once it has attached w/ ptrace
      raise(SIGSTOP);

      // this works, gets exit code resmack::fuzz::asan::ASAN_EXIT_CODE
      pthread_t thread = utils::CreateThread(&ForkLauncher::SpawnThreadTarget, this, tracee);
      pthread_join(thread, nullptr);


      DEBUG_PRINT("child: Done\n");

      _exit(0);
    }

    DEBUG_PRINT("parent: Forked, waiting for SIGSTOP from child\n");
    int tracee_status = 0;
    if (
        waitpid(fork_pid, &tracee_status, 0) != -1
        && WIFSTOPPED(tracee_status)
        && WSTOPSIG(tracee_status) == SIGSTOP
    ) {
      DEBUG_PRINT("parent: Saw the SIGSTOP, sending PTRACE_CONT\n");
      if (ptrace(PTRACE_CONT, fork_pid, nullptr, nullptr) == -1) {
        throw std::runtime_error("Could not tell forked process to continue: " + std::string(std::strerror(errno)));
      }
    } else {
      throw std::runtime_error("Could not wait for forked pid to stop: " + std::string(std::strerror(errno)));
    }

    DEBUG_PRINT("parent: Done spawning, child pid: %d\n", fork_pid);

    return fork_pid;
  }

  void* ForkLauncher::SpawnThreadTarget(Tracee* tracee) {
    DEBUG_PRINT("In spawn thread, calling main fuzz callback\n");
    cb(tracee);
    return nullptr;
  }

}
}
}
