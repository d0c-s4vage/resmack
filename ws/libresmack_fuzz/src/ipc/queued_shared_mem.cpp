#include <functional>
#include <inttypes.h>
#include <stdlib.h>
#include <sys/select.h>

#include "resmack/types.hpp"
#include "resmack/fuzz/ipc/queued_shared_mem.hpp"

namespace resmack {
namespace fuzz {
namespace ipc {
  QueuedSharedMem::QueuedSharedMem() {}

  QueuedSharedMem::QueuedSharedMem(size_t max_size) {
    this->Init(max_size);
  }

  void QueuedSharedMem::Init(size_t max_size) {
    this->shared_mem_.Init(max_size);
  }

  size_t QueuedSharedMem::DataSize() {
    return this->shared_mem_.DataSize();
  }

  void QueuedSharedMem::AddReceiveHandler(uint16_t message_type, IpcMessageHandler handler) {
    this->handlers_[message_type].emplace_back(handler);
  }

  bool QueuedSharedMem::QueueUpdate(uint16_t message_type, size_t data_length, void* data) {
    return this->message_queue_.SendToParent(message_type, data_length, data);
  }

  void QueuedSharedMem::ListenForUpdates() {
    this->listen_thread_ = std::thread([this]() {
      uint16_t message_type;
      size_t data_length;
      void* data;

      this->should_run_ = true;
      while (this->should_run_) {
        // TODO select on the message queue with a timeout
        if (!this->message_queue_.ReadFromChild(&message_type, &data_length, &data)) {
          continue;
        }

        if (!this->handlers_.contains(message_type)) {
          continue;
        }

        printf("Handling a message! type: %u\n", message_type);

        for (auto handler : this->handlers_[message_type]) {
          handler(data_length, data, &this->shared_mem_);
        }
      }
    });
  }

  void QueuedSharedMem::StopListeningForUpdates() {
    this->should_run_ = false;
    this->listen_thread_.join();
  }

} // ipc
} // fuzz
} // resmack
