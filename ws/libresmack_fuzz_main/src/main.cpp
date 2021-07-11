#include "resmack/fuzz/cli/main.hpp"
#include "resmack/fuzz/feedbacks/coverage.hpp"

extern "C" {
  int __lsan_is_turned_off() { return 1; }

  void __sanitizer_cov_trace_pc_guard(uint32_t* guard_var) {
    if (resmack::fuzz::cli::SHUTTING_DOWN) { return; }
    resmack::fuzz::feedbacks::HandleSanitizerCovTracePcGuard(guard_var);
  }

  void __sanitizer_cov_trace_pc_guard_init(uint32_t* start, uint32_t* end) {
    resmack::fuzz::feedbacks::HandleSanitizerCovTracePcGuardInit(start, end);
  }
};

int main(int argc, char** argv) {
  resmack::fuzz::cli::Main(argc, argv);
}
