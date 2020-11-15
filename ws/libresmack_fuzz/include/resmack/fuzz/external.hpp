#ifndef RESMACK_FUZZ_EXTERNAL
#define RESMACK_FUZZ_EXTERNAL


namespace resmack {
namespace fuzz {

  struct ExternalFunctions {
    // Initialize function pointers. Functions that are not available will be set
    // to nullptr.  Do not call this constructor  before ``main()`` has been
    // entered.
    ExternalFunctions();

#define EXT_FUNC(NAME, RETURN_TYPE, FUNC_SIG, WARN)                            \
  RETURN_TYPE(*NAME) FUNC_SIG = nullptr

#include "external_fns.def"

#undef EXT_FUNC
  };

}
}


#endif
