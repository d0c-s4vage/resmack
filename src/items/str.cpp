#include "../item.hpp"
#include "str.hpp"

namespace resmack {
namespace items {

  Str::Str() : min_(1), max_(10), charset_(CHARSET_PUNCTUATION) {}
  Str::Str(uint32_t min, uint32_t max) : min_(min), max_(max), charset_(CHARSET_PUNCTUATION) {}
  Str::Str(uint32_t min, uint32_t max, std::string charset) : min_(min), max_(max), charset_(charset) {}

  Str::~Str() {}

  ItemType Str::Type() {
   return ItemType::STR;
  }

  void Str::Build(BuildContext *ctx) {
   uint32_t range = this->max_ - this->min_;
   if (range > this->max_) {
     throw std::overflow_error(std::string("Overflow with Str max (")
                               + std::to_string(this->max_)
                               + "), min("
                               + std::to_string(this->min_)
                               + ")");
   }
   uint32_t num_chars = this->min_ + (ctx->rand->Next() % range);
   for(uint32_t i = 0; i < num_chars; i++) {
     char c = this->charset_[ctx->rand->Next() % this->charset_.size()];
     ctx->output->push_back(c);
   }
  }

}
}
