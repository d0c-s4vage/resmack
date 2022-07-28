#ifndef RESMACK_FUZZ_DEBUG_H
#define RESMACK_FUZZ_DEBUG_H

//#define _DEBUG_PRINT(...) printf(__VA_ARGS__); fflush(stdout);
#define _DEBUG_PRINT(...)

#ifdef DEBUG_MESSAGES
#define DEBUG_PRINT(...) _DEBUG_PRINT(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

#endif
