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
    uint16_t length;
  };

  class MessageQueue {
   public:
    static const uint16_t TYPE_NO_MESSAGE = 0x1111;

    MessageQueue();

    // Add a message to the queue of type `message_type`, length `length`, and
    // consisting of data `data`
    template <typename T>
    bool SendToParent(uint16_t type, const T* data) {
      MessageHeader header { .type = type, .length = sizeof(T) };
      int rc;
      rc = write(this->child_socket, reinterpret_cast<char*>(&header), sizeof(header));
      if (rc == -1) {
        perror("Could not write to socket");
        std::exit(1);
      }
      if (rc != sizeof(header)) {
        return false;
      }

      rc = write(this->child_socket, (void*)data, sizeof(T));
      if (rc == -1) {
        perror("Could not write to socket");
        std::exit(1);
      }
      if (rc != sizeof(T)) {
        return false;
      }

      return true;
    }


    // Read the message from the queue, returning the
    template<typename T>
    bool ReadFromChild(uint16_t* out_type, T** out_data) {
      MessageHeader header;
      memset(&header, 0, sizeof(header));
      int rc = read(this->parent_socket, &header, sizeof(header));
      if (rc == -1) {
        perror("Could not read from socket");
        std::exit(1);
      }
      if (rc != sizeof(header)) {
        return false;
      }

      *out_type = header.type;

      *out_data = reinterpret_cast<T*>(malloc(header.length));
      if (out_data == nullptr) {
        return false;
      }

      rc = read(this->parent_socket, *out_data, header.length);
      if (rc == -1) {
        perror("Could not read data from socket");
        std::exit(1);
      }
      if (rc != header.length) {
        return false;
      }

      return true;
    }

   private:
    int parent_socket;
    int child_socket;

  };

} // namespace ipc
} // namespace fuzz
} // namespace resmack

#endif
