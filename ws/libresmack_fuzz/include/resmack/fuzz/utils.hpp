#ifndef RESMACK_FUZZ_UTILS_H
#define RESMACK_FUZZ_UTILS_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <openssl/sha.h>
#include <unistd.h>

namespace resmack {
namespace fuzz {
namespace utils {

  [[maybe_unused]]
  static pid_t MAIN_PID;

  [[maybe_unused]]
  static bool KEEP_RUNNING;

  // Caller is responsible for freeing the returned (malloc'd) hex digest
  inline char* sha1_hex(const char* data, size_t data_size, char* out_buffer) {
    if (out_buffer == NULL) {
      out_buffer = (char*)malloc((SHA_DIGEST_LENGTH * 2) + 1);
    }

    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)data, data_size, (unsigned char *)digest);

    for (size_t i = 0; i < SHA_DIGEST_LENGTH; i++) {
      snprintf(out_buffer + (i * 2), 4, "%02x", digest[i]);
    }
    return out_buffer;
  }

}
}
}

#endif
