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

#endif
