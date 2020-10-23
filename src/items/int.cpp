#include <fmt/compile.h>

#include "../item.hpp"
#include "../utils.hpp"
#include "int.hpp"

namespace resmack {
namespace items {

  Int::Int() : min_(1), max_(100) {}
  Int::Int(int64_t min, int64_t max) : min_(min), max_(max) {
   if (!utils::OvSub(this->max_, this->min_, &this->range_)) {
     throw std::overflow_error(std::string("Overflow with Int max (")
                               + std::to_string(this->max_)
                               + "), min("
                               + std::to_string(this->min_)
                               + ")");
   }
  }
  Int::~Int() {}

  ItemType Int::Type() {
   return ItemType::INT;
  }

  void Int::Build(BuildContext *ctx) {
   int64_t res = this->min_ + (ctx->rand->Next() % this->range_);
   *ctx->output += fmt::format_int(res).c_str();
  }

}
}
