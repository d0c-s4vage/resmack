#include "resmack/items/str.hpp"

#include "../utils.hpp"

namespace resmack {
namespace items {

  Str::Str() : Str(1, 10, CHARSET_ALPHA) {}
  Str::Str(uint32_t min, uint32_t max) : Str(min, max, CHARSET_ALPHA) {}
  Str::Str(uint32_t min, uint32_t max, char charset[]) : Str(min, max, charset, sizeof(charset)) {}
  Str::Str(uint32_t min, uint32_t max, char* charset, size_t charset_size):
    min_(min),
    max_(max),
    charset_(charset),
    charset_size_(charset_size)
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
    utils::RandBytes(ctx->rand, this->charset_, this->charset_size_, num_chars, ctx->output);
  }

}
}
