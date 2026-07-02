
#define INIT_SANITIZER_GUARDS(SHUT_DOWN_VAR) \
  extern "C" { \
    ATTRIBUTE_NO_SANITIZING \
    int __lsan_is_turned_off() { return 1; } \
    ATTRIBUTE_NO_SANITIZING \
    ATTRIBUTE_NO_SANITIZE_COVERAGE \
    void __sanitizer_cov_trace_pc_guard(uint32_t* guard_var) { \
      if (SHUT_DOWN_VAR) { return; } \
      resmack::fuzz::HandleSanitizerCovTracePcGuard(guard_var); \
    } \
    ATTRIBUTE_NO_SANITIZING \
    ATTRIBUTE_NO_SANITIZE_COVERAGE \
    void __sanitizer_cov_trace_pc_guard_init(uint32_t* start, uint32_t* end) { \
      resmack::fuzz::HandleSanitizerCovTracePcGuardInit(start, end); \
    } \
  };
