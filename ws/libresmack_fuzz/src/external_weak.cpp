#include <iostream>

#include "resmack/fuzz/external.hpp"

extern "C" {
// Declare these symbols as weak to allow them to be optionally defined.
#define EXT_FUNC(NAME, RETURN_TYPE, FUNC_SIG, WARN)                            \
  __attribute__((weak, visibility("default"))) RETURN_TYPE NAME UNPAREN(FUNC_SIG)

#define EXT_FUNC_CPP(NAME, RETURN_TYPE, FUNC_SIG, WARN)

#include "resmack/fuzz/external_fns.def"

#undef EXT_FUNC
#undef EXT_FUNC_CPP
}

extern "C++" {

#define EXT_FUNC(NAME, RETURN_TYPE, FUNC_SIG, WARN)

// Declare these symbols as weak to allow them to be optionally defined.
#define EXT_FUNC_CPP(NAME, RETURN_TYPE, FUNC_SIG, WARN)                            \
  __attribute__((weak, visibility("default"))) RETURN_TYPE NAME UNPAREN(FUNC_SIG)

#include "resmack/fuzz/external_fns.def"

#undef EXT_FUNC
#undef EXT_FUNC_CPP
}

static void CheckFnPtr(void *fn_ptr, const char *fn_name, bool warn_if_missing) {
  if (fn_ptr == nullptr && warn_if_missing) {
    std::cout << "WARNING: Failed to find function \"" << fn_name << "\"" << std::endl;
  }
}

namespace resmack {
namespace fuzz {

  ExternalFunctions::ExternalFunctions() {
#define EXT_FUNC(NAME, RETURN_TYPE, FUNC_SIG, WARN)                            \
  this->NAME = ::NAME;                                                         \
  CheckFnPtr(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(::NAME)),    \
             #NAME, WARN);
#define EXT_FUNC_CPP(NAME, RETURN_TYPE, FUNC_SIG, WARN)                            \
  this->NAME = ::NAME;                                                         \
  CheckFnPtr(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(::NAME)),    \
             #NAME, WARN);

#include "resmack/fuzz/external_fns.def"

#undef EXT_FUNC
#undef EXT_FUNC_CPP
  }

} 
}
