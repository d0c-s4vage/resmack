#ifndef RESMACK_FUZZ_UTILS_H
#define RESMACK_FUZZ_UTILS_H

#include <barrier>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <mutex>
#include <openssl/sha.h>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace resmack {
namespace fuzz {
namespace utils {
  inline void throw_runtime_error(std::string msg) {
    throw std::runtime_error(msg + ": " + std::string(strerror(errno)));
  }

  inline float GetTimeNow() {
    timespec tmp;
    clock_gettime(CLOCK_MONOTONIC, &tmp);
    return tmp.tv_sec + 1e-9 * tmp.tv_nsec;
  }

  // Caller is responsible for freeing the returned (malloc'd) hex digest
  inline char* sha1_hex(const char* data, size_t data_size, char* out_buffer) {
    if (out_buffer == nullptr) {
      out_buffer = (char*)malloc((SHA_DIGEST_LENGTH * 2) + 1);
    }

    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)data, data_size, (unsigned char *)digest);

    for (size_t i = 0; i < SHA_DIGEST_LENGTH; i++) {
      snprintf(out_buffer + (i * 2), 4, "%02x", digest[i]);
    }
    return out_buffer;
  }

  template <typename Fn, typename... Args>
  pthread_t CreateThread(Fn&& fn, Args&&... args) {
    static std::mutex thread_lock;

    std::scoped_lock _lock(thread_lock);
    std::barrier<> sync(2);

    auto bound = std::make_unique<std::function<void()>>(
      [&sync, fn = std::forward<Fn>(fn), ... captured = std::forward<Args>(args)]() mutable {
        sync.arrive_and_wait();
        std::invoke(fn, captured...);
      }
    );

    // + to convert it into a raw function pointer
    void* (*trampoline)(void*) = +[](void* arg) -> void* {
      std::unique_ptr<std::function<void()>> fn_ptr(static_cast<std::function<void()>*>(arg));
      (*fn_ptr)();
      return nullptr;
    };

    pthread_t thread;
    int res = pthread_create(&thread, nullptr, trampoline, bound.get());
    if (res != 0) {
      throw std::runtime_error(
          "pthread_create failed: " + std::string(std::strerror(res))
          );
    }
    sync.arrive_and_wait();

    // trampoline takes over with the std::unique_ptr
    bound.release();

    return thread;
  }


}
}
}

#endif
