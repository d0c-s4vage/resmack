#ifndef RESMACK_ITEM_REF
#define RESMACK_ITEM_REF

#include <string>

#include "../item.hpp"
#include "../build_context.hpp"
#include "../rules.hpp"

namespace resmack {
namespace items {

class Ref: public resmack::Item {
  private:
   std::string rule_name_;

  public:
   Ref(std::string rule_name) : rule_name_(rule_name) {}
   ~Ref() {}

   ItemType Type() {
     return ItemType::REF;
   }

   void Build(BuildContext *ctx) {
     ctx->rules->Build(this->rule_name_, ctx);
   }
};

}
}

#endif
