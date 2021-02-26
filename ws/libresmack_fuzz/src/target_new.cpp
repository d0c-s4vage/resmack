#include "resmack/fuzz/debug.hpp"
#include "resmack/fuzz/external.hpp"
#include "resmack/fuzz/target_new.hpp"
#include "resmack/fuzz/targets_new/direct.hpp"

namespace resmack {
namespace fuzz {
namespace targets {

  Target* CreateTarget(size_t id, TargetType type, TargetHooks* hooks, size_t max_input_size) {
    ExternalFunctions EF;

    switch (type) {
      case TargetType::kDirect:
        return static_cast<Target*>(new DirectTarget(
          id,
          [id, EF](const char* data, size_t size) -> int {
            _DEBUG_PRINT("%lu: %d calling LLVMFuzzerTestOneInput, EF: %p\n", id, getpid(), EF.LLVMFuzzerTestOneInput);
            return EF.LLVMFuzzerTestOneInput(reinterpret_cast<const uint8_t*>(data), size);
          },
          hooks,
          max_input_size
        ));
        break;
    }
  }

}
}
}
