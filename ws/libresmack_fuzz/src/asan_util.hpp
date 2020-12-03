#ifndef RESMACK_FUZZ_ASAN_UTIL_H
#define RESMACK_FUZZ_ASAN_UTIL_H

#include <algorithm>
#include <sanitizer/asan_interface.h>

__attribute__((weak, visibility("default")))
void __asan_set_error_report_callback(void(*)(const char*));

namespace resmack {
namespace fuzz {
namespace asan {

  static char *ASAN_DEFAULT_OPTIONS = "exitcode=199";
  static int ASAN_EXIT_CODE = 199;

  extern "C"
  __attribute__((no_sanitize_address))
  const char *__asan_default_options();

  using AsanCb = std::function<void(const char*)>;
  static AsanCb ASAN_CB = NULL;

  void HandleAsan(const char* report);

  void SetAsanCallback(AsanCb cb);

}
}
}

#endif
