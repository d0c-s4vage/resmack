#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "resmack/debug.hpp"

#include "resmack/fuzz/asan_util.hpp"
#include "resmack/fuzz/ipc_util.hpp"
#include "resmack/fuzz/lock.hpp"
#include "resmack/fuzz/tracee.hpp"
#include "resmack/fuzz/process_launchers/fork.hpp"

namespace resmack {
namespace fuzz {
namespace process_launchers {

  // install signal handler here to catch SIGINT --> set SHUTTING_DOWN = true
  void sigint_handler([[maybe_unused]] int signum) {
    DEBUG_PRINT("IN SIGINT HANDLER FORK.CPP\n");
    [[maybe_unused]]
    int sem_val = resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK.GetValue();
    if (sem_val == 1) {
      resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK.unlock();
    }
    //resmack::fuzz::ipc_util::SHUTTING_DOWN.store(true);
    _exit(0); // no cleanup!
  }

  ForkLauncher::ForkLauncher(bool mute_io, TraceSpawnCb cb) : cb(cb), mute_io(mute_io) {}
  ForkLauncher::~ForkLauncher() {}

  pid_t ForkLauncher::Spawn(Tracee* tracee) {
    tracee->Reset();

    DEBUG_PRINT("SPAWNING\n");
    pid_t fork_pid = fork();
    if (fork_pid == 0) {
      if (this->mute_io) {
        DEBUG_PRINT("ForkLauncher child: Muting IO\n");
        int fd = open("/dev/null", O_WRONLY);
        fflush(stdout);
        dup2(fd, 1);
        fflush(stderr);
        dup2(fd, 2);
        close(fd);
      }

      if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        DEBUG_PRINT("ForkLauncher child: error installing sigint handler\n");
        _exit(1);
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

      /* We need the actual execution to occur *NOT* on the main thread
       * so that we can gracefully handle signals. Without this, the timeout
       * monitor thread will kill the fuzzing process while it is holding
       * a corpus/state semaphore, which ends up bringing everything down.
       */
      SpawnThreadArgs args {
        .this_ = this,
        .tracee = tracee,
      };
      pthread_t thread;

      DEBUG_PRINT("ForkLauncher child: creating spawn target thread\n");
      pthread_create(&thread, NULL, &SpawnThreadTarget, (void*)&args);

      DEBUG_PRINT("ForkLauncher child: Waiting for spawn target thread to finish\n");
      pthread_join(thread, NULL);
      DEBUG_PRINT("ForkLauncher child: Done, exiting spawn thread\n");

      _exit(0);
    }

    DEBUG_PRINT("ForkLauncher parent: Forked, waiting for SIGSTOP from child\n");
    int tracee_status = 0;
    if (
        waitpid(fork_pid, &tracee_status, 0) != -1
        && WIFSTOPPED(tracee_status)
        && WSTOPSIG(tracee_status) == SIGSTOP
    ) {
      DEBUG_PRINT("ForkLauncher parent: Saw the SIGSTOP, sending PTRACE_CONT\n");
      if (ptrace(PTRACE_CONT, fork_pid, NULL, NULL) == -1) {
        throw std::runtime_error("Could not tell forked process to continue: " + std::string(std::strerror(errno)));
      }
    } else {
      throw std::runtime_error("Could not wait for forked pid to stop: " + std::string(std::strerror(errno)));
    }

    DEBUG_PRINT("ForkLauncher parent: Done spawning, child pid: %d\n", fork_pid);

    return fork_pid;
  }

  void* ForkLauncher::SpawnThreadTarget(void* spawn_thread_args) {

    //ignore SIGINT on this thread! we want the main thread to
    //handle it
    sigset_t signal_mask;
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &signal_mask, NULL) != 0) {
      std::cerr << "Error ignoring SIGINT in worker thread: " + std::string(std::strerror(errno)) << std::endl;
      _exit(1);
    }

    SpawnThreadArgs* args = (SpawnThreadArgs*)spawn_thread_args;
    args->this_->cb(args->tracee);
    return NULL;
  }

}
}
}
