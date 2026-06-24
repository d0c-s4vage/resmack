#include <fcntl.h>
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <unistd.h>

#include "resmack/debug.hpp"

#include "resmack/fuzz/tracee.hpp"
#include "resmack/fuzz/lock.hpp"
#include "resmack/fuzz/trace_targets/fork.hpp"
#include "resmack/fuzz/ipc_util.hpp"

#include "asan_util.hpp"

namespace resmack {
namespace fuzz {
namespace trace_targets {

  // install signal handler here to catch SIGINT --> set SHUTTING_DOWN = true

  void sigint_handler([[maybe_unused]] int signum) {
    DEBUG_PRINT("%d:%d: >>Handling signal handler\n", getpid(), std::this_thread::get_id());

    [[maybe_unused]]
    int sem_val = resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK.GetValue();
    DEBUG_PRINT(
      "%d:%d: >>Waiting to acquire sig lock, curr val: %d\n",
      getpid(),
      std::this_thread::get_id(),
      sem_val
    );
    resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK.Acquire();
    DEBUG_PRINT("%d:%d: >>Acquired lock, exiting without cleanup\n", getpid(), std::this_thread::get_id());
    _exit(0); // no cleanup!
  }

  Fork::Fork(bool mute_io, TraceSpawnCb cb) : cb(cb), mute_io(mute_io) {}
  Fork::~Fork() {}

  pid_t Fork::Spawn(Tracee* tracee) {
    pid_t res;
    if ((res = fork()) == 0) {
      if (this->mute_io) {
        int fd = open("/dev/null", O_WRONLY);
        dup2(fd, 1);
        dup2(fd, 2);
        close(fd);
      }

      //signal(SIGINT, sigint_handler);
      if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("  >Forkee: Could not install sig handler in forked process\n");
        std::cout << std::flush;
        std::cerr << std::flush;
      }

      resmack::fuzz::asan::SetAsanCallback([tracee](const char* report) {
        std::cout << std::flush;
        if (tracee == NULL) { return; }
        tracee->SaveAsanInfo(report);
        // let it die, the tracer knows to look for the ASAN_EXIT_CODE
      });

      ptrace(PTRACE_TRACEME, 0, NULL, NULL);

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

      _exit(0);
    }

    std::cout << std::flush;
    return res;
  }

  void* Fork::SpawnThreadTarget(void* spawn_thread_args) {
    //signal(SIGINT, SIG_IGN);
    // ignore SIGINT on this thread!
    sigset_t signal_mask;
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &signal_mask, NULL) != 0) {
      perror("Error ignoring SIGINT in worker thread\n");
    }

    SpawnThreadArgs* args = (SpawnThreadArgs*)spawn_thread_args;
    args->this_->cb(args->tracee);
    return NULL;
  }

}
}
}
