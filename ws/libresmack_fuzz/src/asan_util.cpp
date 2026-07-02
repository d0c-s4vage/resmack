#include <cstdlib>
#include <iostream>

#include <sanitizer/asan_interface.h>

#include "resmack/debug.hpp"
#include "resmack/fuzz/asan_util.hpp"

namespace resmack {
namespace fuzz {
namespace asan {

void HandleAsan(const char* report) {
  ASAN_CB(report);
}

void SetAsanCallback(AsanCb cb) {
  ASAN_CB = cb;
  if (__asan_set_error_report_callback != NULL) {
    __asan_set_error_report_callback(HandleAsan);
    DEBUG_PRINT("Set the asan error report callback\n");
  } else {
    DEBUG_PRINT("COULD NOT SET ERROR REPORT CALLBACK\n");
  }
}

}
}
}
