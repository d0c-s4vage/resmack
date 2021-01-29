#include <fcntl.h>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <unistd.h>

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
  DEBUG_PRINT("%d: >>Handling signal handler\n", getpid());
  int sem_val;
  if (sem_getvalue(&resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK, &sem_val) == -1) {
    perror("Error getting value for SIGNAL_HANDLER_LOCK");
    std::cerr << std::flush;
    DEBUG_PRINT("%d: >>Error geting semaphore value!\n", getpid());
  }
  DEBUG_PRINT(
    "%d: >>Waiting to acquire sig lock, curr val: %d, inited: %d\n",
    getpid(),
    sem_val,
    resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK_INITED
  );
  if (sem_wait(&resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK) == -1) {
    perror("Error locking SIGNAL_HANDLER_LOCK, exiting up anyways");
    std::cerr << std::flush;
    DEBUG_PRINT("%d: >>Error waiting for semaphore!\n", getpid());
    _exit(0); // no cleanup!
  }
  DEBUG_PRINT("%d: >>Acquired lock, exiting without cleanup\n", getpid());
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

    sem_init(&resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK, 0, 1);
    resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK_INITED = true;
    DEBUG_PRINT("%d: INITED SIGLOCK: %d\n", getpid(), resmack::fuzz::ipc_util::SIGNAL_HANDLER_LOCK_INITED);

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
  // ignore sigint! must only be handled by the main thread
  signal(SIGINT, SIG_IGN);
  SpawnThreadArgs* args = (SpawnThreadArgs*)spawn_thread_args;
  args->this_->cb(args->tracee);
  return NULL;
}

}
}
}
