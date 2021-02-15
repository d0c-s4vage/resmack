#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "resmack/fuzz/external.hpp"
#include "resmack/fuzz/targets_new/direct.hpp"
#include "resmack/fuzz/target_hooks.hpp"

namespace resmack {
namespace fuzz {
namespace targets {

  static ExternalFunctions EF;

  DirectTarget::DirectTarget(
      TargetCb callback,
      TargetHooks* hooks,
      size_t max_input_size
  ) :
    callback(callback),
    hooks(hooks),
    ipc(NULL)
  {
    size_t ipc_max_size = hooks->ExecAndSumIpcSize();
    ipc_max_size += sizeof(DirectTargetIpcInfo) + max_input_size - 1;
    this->ipc_memory.Init(ipc_max_size);

    this->ipc = this->ipc_memory.GetNextPtrFor<DirectTargetIpcInfo>(
      sizeof(DirectTargetIpcInfo) + max_input_size - 1
    );

    this->ipc->input_ready.Init();
    this->ipc->input_processed.Init();

    hooks->ExecIpcInit(&this->ipc_memory);
  }

  DirectTarget::~DirectTarget() {}

  pid_t DirectTarget::Start() {
    this->hooks->ExecPreStart(&this->ipc_memory);

    this->ipc->input_ready.Reset();
    this->ipc->input_processed.Reset();

    this->running_target = fork();
    if (this->running_target == -1) {
      perror("Could not fork!");
      std::exit(1);
    }

    if (this->running_target == 0) {
      this->hooks->ExecPreStartInTarget(&this->ipc_memory);
      this->TestLoop();
      _exit(0);
    }

    this->hooks->ExecPostStart(&this->ipc_memory, this->running_target, this);

    return this->running_target;
  }

  void DirectTarget::Stop() {
    if (this->running_target == -1) { return; }

    this->hooks->ExecPreStop(&this->ipc_memory, this->running_target);
    kill(this->running_target, SIGKILL);
    waitpid(this->running_target, NULL, 0);
    this->hooks->ExecPostStop(&this->ipc_memory, this->running_target);
    this->running_target = -1;
  }

  int DirectTarget::Test(const std::string* input) {
    this->hooks->ExecPreTest(&this->ipc_memory);

    this->ipc->input_ready.Lock();
    memcpy(&this->ipc->data, input->data(), input->size());
    this->ipc->data_size = input->size();
    this->ipc->result = -1;

    this->ipc->input_ready.SignalRaw_Danger();
    this->ipc->input_ready.Unlock();

    this->ipc->input_processed.WaitAndHold();
    int res = this->ipc->result;
    this->ipc->input_processed.Unlock();

    return res;
  }

  void DirectTarget::ForceFinishTest() {
    this->ipc->input_processed.SignalRaw_Danger();
    this->ipc->input_processed.Unlock();
  }

  // --------------------------------------------------------------------------

  void DirectTarget::TestLoop() {
    while (true) {
      this->ipc->input_ready.WaitAndHold();
        char* data = &this->ipc->data;
        size_t data_size = this->ipc->data_size;
        // Call the target function!
        int res = this->callback(data, data_size);
      this->ipc->input_ready.Unlock();

      this->ipc->input_processed.Lock();
        this->ipc->result = res;
      this->ipc->input_processed.SignalRaw_Danger();
      this->ipc->input_processed.Unlock();
    }
  }

}
}
}
