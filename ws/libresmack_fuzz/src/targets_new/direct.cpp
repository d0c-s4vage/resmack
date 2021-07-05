#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "resmack/fuzz/debug.hpp"
#include "resmack/fuzz/external.hpp"
#include "resmack/fuzz/targets_new/direct.hpp"
#include "resmack/fuzz/target_hooks.hpp"

namespace resmack {
namespace fuzz {
namespace targets {

  static ExternalFunctions EF;

  DirectTarget::DirectTarget(
      size_t id,
      TargetCb callback,
      TargetHooks* hooks,
      size_t max_input_size
  ) :
    id(id),
    max_input_size(max_input_size),
    callback(callback),
    hooks(hooks),
    ipc(NULL)
  {
    size_t ipc_max_size = hooks->ExecAndSumIpcSize();
    ipc_max_size += sizeof(DirectTargetIpcInfo) + max_input_size - 1;
    this->ipc_memory.Init(ipc_max_size);

    this->ipc = this->ipc_memory.GetNextPtrFor<DirectTargetIpcInfo>(
      sizeof(DirectTargetIpcInfo) + max_input_size // -1  // yes, leave an extra byte at the end for a null terminator
    );

    hooks->ExecIpcInit(&this->ipc_memory);
  }

  DirectTarget::~DirectTarget() {}

  pid_t DirectTarget::Start() {
    _DEBUG_PRINT("%lu: DirectTarget: Starting\n", this->id);
    this->hooks->ExecPreStart(&this->ipc_memory);

    _DEBUG_PRINT("%lu: DirectTarget: Forking\n", this->id);
    this->running_target = fork();
    if (this->running_target == -1) {
      _DEBUG_PRINT("%lu: DirectTarget: Could not fork! %s\n", this->id, strerror(errno));
      fflush(stdout);
      std::exit(1);
    }

    if (this->running_target == 0) {
      _DEBUG_PRINT("%lu: %d DirectTarget: In fork, executing pre start in target\n", this->id, getpid());
      this->hooks->ExecPreStartInTarget(&this->ipc_memory);
      _DEBUG_PRINT("%lu: %d DirectTarget: In fork, done executing pre start in target\n", this->id, getpid());
      this->TestLoop();
      _exit(0);
    }

    _DEBUG_PRINT("%lu: DirectTarget: Forked, child proc: %d\n", this->id, this->running_target);
    this->hooks->ExecPostStart(&this->ipc_memory, this->running_target, this);

    return this->running_target;
  }

  void DirectTarget::Stop() {
    if (this->running_target == -1) { return; }

    _DEBUG_PRINT("%lu: Stopping the target\n", this->id);
    this->hooks->ExecPreStop(&this->ipc_memory, this->running_target);
    kill(this->running_target, SIGKILL);
    waitpid(this->running_target, NULL, 0);
    this->hooks->ExecPostStop(&this->ipc_memory, this->running_target);
    this->running_target = -1;
  }

  // --------------------------------------------------------------------------

  void DirectTarget::TestLoop() {
    while (true) {
      std::string const* data = this->genr->Generate();
      // TODO do something with the return value?
      this->callback(data->data(), data->size());
    }
    printf("%lu: %d Completely finished loop somehow\n", this->id, getpid());
  }

}
}
}
