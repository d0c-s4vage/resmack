#include <cstdlib>
#include <fcntl.h>
#include <iostream>
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
#include "resmack/fuzz/tracee.hpp"
#include "resmack/fuzz/lock.hpp"
#include "resmack/fuzz/trace_targets/fork.hpp"
#include "resmack/fuzz/ipc_util.hpp"

namespace resmack {
namespace fuzz {
namespace trace_targets {

  // install signal handler here to catch SIGINT --> set SHUTTING_DOWN = true
  void sigint_handler([[maybe_unused]] int signum) {
    [[maybe_unused]]
    int sem_val = resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK.GetValue();
    if (sem_val == 1) {
      resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK.unlock();
    }
    _exit(0); // no cleanup!
  }

  Fork::Fork(bool mute_io, TraceSpawnCb cb) : cb(cb), mute_io(mute_io) {}
  Fork::~Fork() {}

  pid_t Fork::Spawn(Tracee* tracee) {
    pid_t fork_pid;

    /* The parent process will block on a read() of these pipes to continue -
     * this will give the forked process a chance to start up.
     *
     * This is an attempt to deal with a race condition involving
     * pthreads, ptrace (and maybe ASAN), and __futex_abstimed_wait_common64
     * blocking everything. When the race condition / deadlock occurs,
     * the child process is created, never runs, and externally it will look
     * like the Tracer::Join() call blocks infinitely.
     */
    if ((fork_pid = fork()) == 0) {

      if (this->mute_io) {
        int fd = open("/dev/null", O_WRONLY);
        fflush(stdout);
        dup2(fd, 1);
        fflush(stderr);
        dup2(fd, 2);
        close(fd);
      }

      if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("  >Forkee: Could not install sig handler in forked process");
        std::cout << std::flush;
        std::cerr << std::flush;
      }

      resmack::fuzz::asan::SetAsanCallback([tracee](const char* report) {
        std::cout << std::flush;
        if (tracee == NULL) { return; }
        tracee->SaveAsanInfo(report);
        // let it die, the tracer knows to look for the ASAN_EXIT_CODE
      });

      if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
        perror("Forked process could not call PTRACE_TRACEME");
      }
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
      pthread_create(&thread, NULL, &SpawnThreadTarget, (void*)&args);
      pthread_join(thread, NULL);
      //SpawnThreadTarget((void*)&args);


      _exit(0);
    }

    int tracee_status = 0;
    if (
        waitpid(fork_pid, &tracee_status, 0) != -1
        && WIFSTOPPED(tracee_status)
        && WSTOPSIG(tracee_status) == SIGSTOP
    ) {
      if (ptrace(PTRACE_CONT, fork_pid, NULL, NULL) == -1) {
        perror("Could not tell forked process to continue");
      }
    } else {
      perror("Could not wait for forked pid to stop");
    }

    return fork_pid;
  }

  __attribute__((noinline))
  void* Fork::SpawnThreadTarget(void* spawn_thread_args) {

    //ignore SIGINT on this thread! we want the main thread to
    //handle it
    sigset_t signal_mask;
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &signal_mask, NULL) != 0) {
      perror("Error ignoring SIGINT in worker thread");
    }

    SpawnThreadArgs* args = (SpawnThreadArgs*)spawn_thread_args;
    args->this_->cb(args->tracee);
    return NULL;
  }

}
}
}
