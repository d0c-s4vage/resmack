#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "resmack/fuzz/external.hpp"
#include "resmack/fuzz/targets_new/direct.hpp"
#include "resmack/fuzz/target_hooks.hpp"

namespace resmack {
namespace fuzz {
namespace targets {

  static ExternalFunctions EF;

  DirectTarget::DirectTarget(
      TargetCb callback,
      TargetHooks hooks,
      size_t max_input_size
  ) :
    callback(callback),
    hooks(hooks)
  {
    size_t ipc_max_size = hooks.ExecAndSumIpcSize();
    ipc_max_size += sizeof(ipc::SharedMemCondition);
    ipc_max_size += sizeof(ipc::SharedMemCondition);
    ipc_max_size += sizeof(size_t);
    ipc_max_size += sizeof(int);
    ipc_max_size += max_input_size;

    this->ipc_memory.Init(ipc_max_size);

    this->input_ready = this->ipc_memory.GetNextPtrFor<ipc::SharedMemCondition>();
    this->input_processed = this->ipc_memory.GetNextPtrFor<ipc::SharedMemCondition>();
    this->ipc_data_size = this->ipc_memory.GetNextPtrFor<size_t>();
    this->ipc_result = this->ipc_memory.GetNextPtrFor<int>();
    this->ipc_data = this->ipc_memory.GetNextPtrFor<char>(max_input_size);

    this->input_ready->Init();
    this->input_processed->Init();

    hooks.ExecIpcInit(&this->ipc_memory);
  }

  DirectTarget::~DirectTarget() {}

  pid_t DirectTarget::Start() {
    this->hooks.ExecPreStart(&this->ipc_memory);

    this->input_ready->Reset();
    this->input_processed->Reset();

    this->running_target = fork();
    if (this->running_target == -1) {
      perror("Could not fork!");
      std::exit(1);
    }

    if (this->running_target == 0) {
      this->hooks.ExecPreStartInTarget(&this->ipc_memory);
      this->TestLoop();
      _exit(0);
    }

    this->hooks.ExecPostStart(&this->ipc_memory, this->running_target);

    return this->running_target;
  }

  void DirectTarget::Stop() {
    this->hooks.ExecPreStop(&this->ipc_memory, this->running_target);
    kill(this->running_target, SIGKILL);
    this->hooks.ExecPostStop(&this->ipc_memory, this->running_target);
  }

  int DirectTarget::Test(const std::string* input) {
    this->hooks.ExecPreTest(&this->ipc_memory);

    this->input_ready->Lock();
    memcpy(this->ipc_data, input->data(), input->size());
    *this->ipc_data_size = input->size();
    *this->ipc_result = -1;

    this->input_ready->SignalRaw_Danger();
    this->input_ready->Unlock();

    this->input_processed->WaitAndHold();
    int res = *this->ipc_result;
    this->input_processed->Unlock();

    return res;
  }

  // --------------------------------------------------------------------------

  void DirectTarget::TestLoop() {
    while (true) {
      this->input_ready->WaitAndHold();
        char* data = this->ipc_data;
        size_t data_size = *this->ipc_data_size;
        // Call the target function!
        int res = this->callback(data, data_size);
      this->input_ready->Unlock();

      this->input_processed->Lock();
        *this->ipc_result = res;
      this->input_processed->SignalRaw_Danger();
      this->input_processed->Unlock();
    }
  }

}
}
}
