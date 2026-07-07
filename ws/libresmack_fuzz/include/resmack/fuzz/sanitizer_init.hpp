
#define INIT_SANITIZER_GUARDS \
  extern "C" { \
    int __lsan_is_turned_off() { return 1; } \
    void __sanitizer_cov_trace_pc_guard(uint32_t* guard_var) { \
      resmack::fuzz::HandleSanitizerCovTracePcGuard(guard_var); \
    } \
    void __sanitizer_cov_trace_pc_guard_init(uint32_t* start, uint32_t* end) { \
      resmack::fuzz::HandleSanitizerCovTracePcGuardInit(start, end); \
    } \
  };
