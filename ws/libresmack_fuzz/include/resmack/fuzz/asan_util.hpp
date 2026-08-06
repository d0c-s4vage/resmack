#ifndef RESMACK_FUZZ_ASAN_UTIL_H
#define RESMACK_FUZZ_ASAN_UTIL_H
//#pragma once

#include <functional>
#include <sanitizer/asan_interface.h>
#include <sanitizer/coverage_interface.h>

#include "resmack/defs.hpp"

// From gtest/gtest.h
//
// A function level attribute to disable checking for use of uninitialized
// memory when built with MemorySanitizer.

#if defined(__clang__)
# if __has_feature(address_sanitizer)
#  define ATTRIBUTE_NO_SANITIZING\
  __attribute__((disable_sanitizer_instrumentation))
# else
#  define ATTRIBUTE_NO_SANITIZING
# endif  // __has_feature(address_sanitizer)
#else
# define ATTRIBUTE_NO_SANITIZING
#endif  // __clang__


#if defined(__clang__)
# if __has_feature(coverage_sanitizer)
#  define ATTRIBUTE_NO_SANITIZE_COVERAGE\
       __attribute__((no_sanitize("coverage")))
# else
#  define ATTRIBUTE_NO_SANITIZE_COVERAGE
# endif  // __has_feature(coverage_sanitizer)
#else
# define ATTRIBUTE_NO_SANITIZE_COVERAGE
#endif  // __clang__


#if defined(__clang__)
# if __has_feature(memory_sanitizer)
#  define ATTRIBUTE_NO_SANITIZE_MEMORY\
       __attribute__((no_sanitize_memory))
# else
#  define ATTRIBUTE_NO_SANITIZE_MEMORY
# endif  // __has_feature(memory_sanitizer)
#else
# define ATTRIBUTE_NO_SANITIZE_MEMORY
#endif  // __clang__

// A function level attribute to disable AddressSanitizer instrumentation.
#if defined(__clang__)
# if __has_feature(address_sanitizer)
#  define ATTRIBUTE_NO_SANITIZE_ADDRESS\
       __attribute__((no_sanitize_address))
# else
#  define ATTRIBUTE_NO_SANITIZE_ADDRESS
# endif  // __has_feature(address_sanitizer)
#else
# define ATTRIBUTE_NO_SANITIZE_ADDRESS
#endif  // __clang__

// A function level attribute to disable HWAddressSanitizer instrumentation.
#if defined(__clang__)
# if __has_feature(hwaddress_sanitizer)
#  define ATTRIBUTE_NO_SANITIZE_HWADDRESS\
       __attribute__((no_sanitize("hwaddress")))
# else
#  define ATTRIBUTE_NO_SANITIZE_HWADDRESS
# endif  // __has_feature(hwaddress_sanitizer)
#else
# define ATTRIBUTE_NO_SANITIZE_HWADDRESS
#endif  // __clang__

// A function level attribute to disable ThreadSanitizer instrumentation.
#if defined(__clang__)
# if __has_feature(thread_sanitizer)
#  define ATTRIBUTE_NO_SANITIZE_THREAD\
       __attribute__((no_sanitize_thread))
# else
#  define ATTRIBUTE_NO_SANITIZE_THREAD
# endif  // __has_feature(thread_sanitizer)
#else
# define ATTRIBUTE_NO_SANITIZE_THREAD
#endif  // __clang__

extern "C" {
    __attribute__((weak, visibility("default")))
    void __asan_set_error_report_callback(void(*)(const char*));

    __attribute__((visibility("default")))
    const char *__asan_default_options();

    __attribute__((visibility("default")))
    int __lsan_is_turned_off();
}

namespace resmack {
namespace fuzz {
namespace asan {

  inline constexpr int ASAN_EXIT_CODE = 199;
  inline constexpr const char *ASAN_DEFAULT_OPTIONS = "exitcode=199:detect_leaks=0:halt_on_error=1:verbosity=0:symbolize=1";
  // symbolize=0
  // log_path=/dev/null
  //const char *ASAN_DEFAULT_OPTIONS = "exitcode=199:detect_leaks=0:symbolize=0:halt_on_error=1";

  using AsanCb = std::function<void(const char*)>;
  inline AsanCb ASAN_CB = NULL;

  void InitAsanOptions();
  void HandleAsan(const char* report);
  void SetAsanCallback(AsanCb cb);

}
}
}

#endif
