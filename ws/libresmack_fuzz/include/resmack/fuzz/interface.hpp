#ifndef RESMACK_FUZZ_INTERFACE
#define RESMACK_FUZZ_INTERFACE

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// Define FUZZER_INTERFACE_VISIBILITY to set default visibility in a way that
// doesn't break MSVC.
#if defined(_WIN32)
#define FUZZER_INTERFACE_VISIBILITY __declspec(dllexport)
#else
#define FUZZER_INTERFACE_VISIBILITY __attribute__((visibility("default")))
#endif

FUZZER_INTERFACE_VISIBILITY int
LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size);


#ifdef __cplusplus
}
#endif  // __cplusplus

#define FLAGGIFIED(NAME) FLAG_ ## NAME


// Two scenarios for these flags:
//
// 1. libresmack_fuzz_main.a - main.cpp
//   * Doesn't have -DFINAL set, so all of the flags are defined as
//     weak symbols (they're undefined in libresmack_fuzz_main.a)
//
// 2. The CLI command `resmack cc` will define FINAL and any flag values
#ifdef FINAL
  
#define DYNAMIC_FLAG(FLAG_NAME) \
  bool FLAG_NAME = FLAGGIFIED(FLAG_NAME);


// this branch is for main.cpp in libresmack_fuzz_main
# else
#define DYNAMIC_FLAG(FLAG_NAME) \
  __attribute__((weak)) bool FLAG_NAME;

#endif // #ifdef FINAL

namespace resmack::fuzz::flags {
  DYNAMIC_FLAG(USING_ASAN);
}

#endif
