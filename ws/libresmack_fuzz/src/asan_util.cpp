#include <cstdlib>
#include <string.h>

#include <sanitizer/asan_interface.h>

#include "resmack/debug.hpp"
#include "resmack/fuzz/asan_util.hpp"


namespace resmack {
namespace fuzz {
namespace asan {

void InitAsanOptions() {
  std::string new_val = "";

  const char* env_val = std::getenv("ASAN_OPTIONS");
  if (NULL != env_val) {
    new_val += env_val;
    new_val += ":";
  }

  new_val += resmack::fuzz::asan::ASAN_DEFAULT_OPTIONS;

  printf("Setting asan options to: %s\n", new_val.c_str());
  if (setenv("ASAN_OPTIONS", new_val.c_str(), 1 /*replace*/) != 0) {
    throw std::runtime_error("Could not set ASAN_OPTIONS environment variable: " + std::string(strerror(errno)));
  }
}

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
