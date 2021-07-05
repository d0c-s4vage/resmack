#ifndef RESMACK_FUZZ_IPC_QUEUED_SHARED_MEM_H
#define RESMACK_FUZZ_IPC_QUEUED_SHARED_MEM_H

#include <functional>
#include <inttypes.h>
#include <stdlib.h>
#include <thread>

#include "resmack/types.hpp"
#include "resmack/fuzz/ipc/message_queue.hpp"
#include "resmack/fuzz/ipc/locked_shared_mem.hpp"

namespace resmack {
namespace fuzz {
namespace ipc {
  using IpcMessageHandler = std::function<void(size_t data_length, void* data, ipc::LockedSharedMem *ipc_mem)>;

  class QueuedSharedMem {
   public:
    QueuedSharedMem();
    QueuedSharedMem(size_t max_size);
    void Init(size_t max_size);
    void AddReceiveHandler(uint16_t message_type, IpcMessageHandler handler);
    bool QueueUpdate(uint16_t message_type, size_t data_length, void* data);
    template <typename T>
    bool QueueUpdate(uint16_t type, const T* data) {
      return this->QueueUpdate(type, sizeof(T), reinterpret_cast<void*>(const_cast<T*>(data)));
    }

    void ListenForUpdates();
    void StopListeningForUpdates();
    size_t DataSize();

    // ------------------------------------------------------------------------
    // These functions are forwarded directly to the LockedSharedMem
    // ------------------------------------------------------------------------

    // Return the pointer to the shared memory, AFTER any bookkeeping
    // structures
    template <typename T>
    T* GetPtr() {
      return this->shared_mem_.GetPtr<T>();
    }

    // A convenience function when identifying offsets within shared_
    template <typename T>
    T* GetNextPtrFor(size_t size) {
      return this->shared_mem_.GetNextPtrFor<T>(size);
    }

    // A convenience function when identifying offsets within shared_
    template <typename T>
    T* GetNextPtrFor() {
      return this->shared_mem_.GetNextPtrFor<T>();
    }
    
   private:
    std::thread listen_thread_;
    MessageQueue message_queue_;
    LockedSharedMem shared_mem_;
    std::unordered_map<uint16_t, Vector<IpcMessageHandler>> handlers_;
    bool should_run_;
  };

} // ipc
} // fuzz
} // resmack

#endif RESMACK_FUZZ_IPC_QUEUED_SHARED_MEM_H
