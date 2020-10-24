#include "str.hpp"
#include "../utils.hpp"

namespace resmack {
namespace items {

  Str::Str() : Str(1, 10, CHARSET_ALPHA) {}
  Str::Str(uint32_t min, uint32_t max) : Str(min, max, CHARSET_ALPHA) {}
  Str::Str(uint32_t min, uint32_t max, std::string charset):
    min_(min),
    max_(max),
    charset_(charset)
  {
    if (!utils::OvSub(this->max_, this->min_, &this->range_)) {
      throw std::overflow_error(std::string("Overflow with Int max (")
          + std::to_string(this->max_)
          + "), min("
          + std::to_string(this->min_)
          + ")");
    }
  }

  Str::~Str() {}

  ItemType Str::Type() {
    return ItemType::STR;
  }

  void Str::Build(BuildContext *ctx) {
    uint32_t num_chars = this->min_ + (ctx->rand->Next() % this->range_);
    utils::RandBytes(ctx->rand, this->charset_, num_chars, ctx->output);
  }

}
}
