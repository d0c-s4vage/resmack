#include "resmack/fuzz/asan_util.hpp"
#include "resmack/fuzz/feedbacks/coverage.hpp"

const char* __asan_default_options() {
  return resmack::fuzz::asan::ASAN_DEFAULT_OPTIONS;
}

int __lsan_is_turned_off() {
  return 1;
}

void __sanitizer_cov_trace_pc_guard_init(uint32_t* start, uint32_t* end) {
  resmack::fuzz::HandleSanitizerCovTracePcGuardInit(start, end);
}

void __sanitizer_cov_trace_pc_guard(uint32_t* guard_var) {
  resmack::fuzz::HandleSanitizerCovTracePcGuard(guard_var);
}
