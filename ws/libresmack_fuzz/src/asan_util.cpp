#include <algorithm>
#include <sanitizer/asan_interface.h>

#include "asan_util.hpp"

namespace resmack {
namespace fuzz {
namespace asan {

__attribute__((no_sanitize_address))
const char *__asan_default_options() {
  return ASAN_DEFAULT_OPTIONS;
}

void HandleAsan(const char* report) {
  ASAN_CB(report);
}

void SetAsanCallback(AsanCb cb) {
  ASAN_CB = cb;
  __asan_set_error_report_callback(HandleAsan);
}

}
}
}
