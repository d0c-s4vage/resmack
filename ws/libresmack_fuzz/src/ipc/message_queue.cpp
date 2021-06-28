#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "resmack/fuzz/ipc/message_queue.hpp"

namespace resmack {
namespace fuzz {
namespace ipc {

  MessageQueue::MessageQueue() {
    int sockets[2];
    int rc = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    if (rc != 0) {
      perror("Could not create socket pair");
      std::exit(1);
    }

    this->parent_socket = sockets[0];
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000; // 100 milliseconds
    setsockopt(this->parent_socket,
               SOL_SOCKET,
               SO_RCVTIMEO,
               reinterpret_cast<const char*>(&tv), sizeof(tv)); 

    this->child_socket = sockets[1];
  }

  bool MessageQueue::SendToParent(
      uint16_t type,
      size_t length,
      void* data
  ) {
    MessageHeader header { .type = type, .length = length };
    int rc;
    rc = write(this->child_socket, reinterpret_cast<char*>(&header), sizeof(header));
    if (rc == -1) {
      perror("Could not write to socket");
      std::exit(1);
    }
    if (rc != sizeof(header)) {
      return false;
    }

    rc = write(this->child_socket, (void*)data, length);
    if (rc == -1) {
      perror("Could not write to socket");
      std::exit(1);
    }
    if (rc != length) {
      return false;
    }

    return true;
  }

  bool MessageQueue::ReadFromChild(
      uint16_t* out_type,
      size_t* out_length,
      void** out_data
  ) {
    MessageHeader header;
    memset(&header, 0, sizeof(header));
    int rc = read(this->parent_socket, &header, sizeof(header));
    if (rc == -1) {
      // socket timeout reached
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return false;
      }
      perror("Could not read from socket");
      std::exit(1);
    }
    if (rc != sizeof(header)) {
      return false;
    }

    *out_type = header.type;
    *out_length = header.length;

    *out_data = malloc(header.length);
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

} // namespace ipc
} // namespace fuzz
} // namespace resmack
