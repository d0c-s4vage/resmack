#ifndef RESMACK_FUZZ_MSG_QUEUE
#define RESMACK_FUZZ_MSG_QUEUE

#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <inttypes.h>

namespace resmack {
namespace fuzz {
namespace ipc {

  struct MessageHeader {
    uint16_t type;
    size_t length;
  };

  class MessageQueue {
   public:
    static const uint16_t TYPE_NO_MESSAGE = 0x1111;

    MessageQueue();


    // Add a message to the queue of type `message_type`, length `length`, and
    // consisting of data `data`
    bool SendToParent(uint16_t type, size_t length, void* data);

    template <typename T>
    bool SendToParent(uint16_t type, const T* data) {
      return this->SendToParent(type, sizeof(T), reinterpret_cast<void*>(const_cast<T*>(data)));
    }

    bool ReadFromChild(uint16_t* out_type, size_t* out_length, void** out_data);

   private:
    int parent_socket;
    int child_socket;

  };

} // namespace ipc
} // namespace fuzz
} // namespace resmack

#endif
