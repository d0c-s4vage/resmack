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
    int rc = socketpair(AF_UNIX, SOCK_DGRAM, 0, sockets);
    if (rc != 0) {
      perror("Could not create socket pair");
      std::exit(1);
    }

    this->parent_socket = sockets[0];
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000; // 100 milliseconds

    /*
    setsockopt(this->parent_socket,
               SOL_SOCKET,
               SO_RCVTIMEO,
               reinterpret_cast<const char*>(&tv), sizeof(tv)); 
               */

    this->child_socket = sockets[1];

    shutdown(this->parent_socket, SHUT_WR);
    shutdown(this->child_socket, SHUT_RD);
  }

  void HexDump(char* data, size_t length, const char* prefix) {
    printf("%s", prefix);
    for (size_t i = 0; i < length; i++) {
      if (i != 0 && i % 0x10 == 0) {
        printf("\n%s", prefix);
      }
      printf("%02x ", (unsigned char)data[i]);
    }
    printf("\n");
  }

  bool MessageQueue::SendToParent(
      uint16_t type,
      size_t length,
      void* data
  ) {
    static uint16_t counter = MAGIC;
    counter++;
    MessageHeader header { .magic = counter, .type = type, .length = length };
    int rc;

    static char buffer[0x4000];
    char* send_buffer;
    size_t total_size = sizeof(header) + length;

    if (total_size > sizeof(buffer)) {
      send_buffer = (char*)malloc(total_size);
    } else {
      send_buffer = buffer;
    }

    memcpy(send_buffer, &header, sizeof(header));
    memcpy(&(send_buffer[sizeof(header)]), data, length);

    //HexDump(send_buffer, total_size, "SENDING: ");

    rc = write(this->child_socket, send_buffer, total_size);

    if (send_buffer != buffer) {
      free(send_buffer);
    }

    if (rc == -1) {
      perror("Could not write to socket");
      std::exit(1);
    }
    if ((size_t)rc != total_size) {
      return false;
    }

    return true;
  }

  bool MessageQueue::ReadFromChild(
      uint16_t* out_type,
      size_t* out_length,
      void** out_data
  ) {
    static char read_buffer[0x10000];

    int rc = read(this->parent_socket, read_buffer, sizeof(read_buffer));
    if (rc == -1) {
      // socket timeout reached
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return false;
      }
      perror("Could not read from socket");
      std::exit(1);
    }
    if ((size_t)rc < sizeof(MessageHeader)) {
      return false;
    }

    MessageHeader* header = reinterpret_cast<MessageHeader*>(read_buffer);

#ifdef MESSAGE_QUEUE_DEBUG
    printf(">>> Received msg, magic: %04x, type: %04x, length: %lu\n", header->magic, header->type, header->length);
    printf(">>> Header:\n");
    HexDump((char*)header, sizeof(MessageHeader), ">>>     ");
#endif

    *out_type = header->type;
    *out_length = header->length;

    *out_data = &read_buffer[sizeof(MessageHeader)];

#ifdef MESSAGE_QUEUE_DEBUG
    printf(">>> Nested data:\n");
    HexDump((char*)*out_data, header->length, ">>>     ");
#endif

    return true;
  }

} // namespace ipc
} // namespace fuzz
} // namespace resmack
