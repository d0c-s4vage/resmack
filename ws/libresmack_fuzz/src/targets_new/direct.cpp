#include <fcntl.h>
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
      size_t max_input_size,
      Generator* genr
  ) :
    id(id),
    max_input_size(max_input_size),
    callback(callback),
    hooks(hooks),
    genr(genr)
  {
    size_t global_ipc_size = hooks->ExecAndSumGlobalIpcSize();

    // DirectTarget should be created *before* any forking occurs so that
    // the global memory will be shared by all
    this->global_mem.Init(global_ipc_size);

    this->hooks->ExecGlobalIpcInit(&this->global_mem);
  }

  DirectTarget::~DirectTarget() {}

  bool DirectTarget::InitPrivateMem() {
    size_t private_ipc_size = hooks->ExecAndSumPrivateIpcSize();
    this->private_mem.Init(private_ipc_size);
    this->hooks->ExecPrivateIpcInit(&this->private_mem);
    return true;
  }

  pid_t DirectTarget::Start() {
    static bool inited = this->InitPrivateMem();

    _DEBUG_PRINT("%lu: DirectTarget: Starting\n", this->id);
    this->hooks->ExecPreStart(&this->private_mem, &this->global_mem);

    _DEBUG_PRINT("%lu: DirectTarget: Forking\n", this->id);
    this->running_target = fork();
    if (this->running_target == -1) {
      _DEBUG_PRINT("%lu: DirectTarget: Could not fork! %s\n", this->id, strerror(errno));
      fflush(stdout);
      std::exit(1);
    }

    if (this->running_target == 0) {
      // TODO if mute
      int fd = open("/dev/null", O_WRONLY);
      dup2(fd, 1);
      dup2(fd, 2);
      close(fd);

      _DEBUG_PRINT("%lu: %d DirectTarget: In fork, executing pre start in target\n", this->id, getpid());
      this->hooks->ExecPreStartInTarget(&this->private_mem, &this->global_mem);
      _DEBUG_PRINT("%lu: %d DirectTarget: In fork, done executing pre start in target\n", this->id, getpid());
      this->TestLoop();
      _exit(0);
    }

    _DEBUG_PRINT("%lu: DirectTarget: Forked, child proc: %d\n", this->id, this->running_target);
    this->hooks->ExecPostStart(&this->private_mem, &this->global_mem, this->running_target, this);

    return this->running_target;
  }

  void DirectTarget::Stop() {
    if (this->running_target == -1) { return; }

    this->hooks->ExecPreStop(&this->private_mem, &this->global_mem, this->running_target);
    kill(this->running_target, SIGKILL);
    waitpid(this->running_target, NULL, 0);
    this->hooks->ExecPostStop(&this->private_mem, &this->global_mem, this->running_target);
    this->running_target = -1;
  }

  // --------------------------------------------------------------------------

  void DirectTarget::TestLoop() {
    while (true) {
      std::string const* data = this->genr->Generate();
      this->hooks->ExecPreTest(&this->private_mem, &this->global_mem);
      // TODO do something with the return value?
      this->callback(data->data(), data->size());
      this->hooks->ExecPostTest(&this->private_mem, &this->global_mem);
    }
  }

}
}
}
