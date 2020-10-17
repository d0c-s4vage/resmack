#include "../item.hpp"
#include "../utils.hpp"
#include "int.hpp"

namespace resmack {
namespace items {

  Int::Int() : min_(1), max_(100) {}
  Int::Int(int64_t min, int64_t max) : min_(min), max_(max) {}
  Int::~Int() {}

  ItemType Int::Type() {
   return ItemType::INT;
  }

  void Int::Build(BuildContext *ctx) {
   int64_t range;
   if (!utils::OvSub(this->max_, this->min_, &range)) {
     throw std::overflow_error(std::string("Overflow with Int max (")
                               + std::to_string(this->max_)
                               + "), min("
                               + std::to_string(this->min_)
                               + ")");
   }
   int64_t res = this->min_ + (ctx->rand->Next() % range);
   ctx->output->append(std::to_string(res));
  }

}
}
