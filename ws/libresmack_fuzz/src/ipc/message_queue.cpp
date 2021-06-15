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
    this->child_socket = sockets[1];
  }

} // namespace ipc
} // namespace fuzz
} // namespace resmack
