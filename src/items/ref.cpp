#include "../item.hpp"
#include "ref.hpp"
#include "../rules.hpp"

namespace resmack {
namespace items {

  Ref::Ref(std::string rule_name) : rule_name_(rule_name) {}
  Ref::~Ref() {}

  ItemType Ref::Type() {
    return ItemType::REF;
  }

  void Ref::Build(BuildContext *ctx) {
    ctx->rules->Build(this->rule_name_, ctx);
  }

  bool Ref::CalcReachability(calc::Reach* reach_calc) {
    return reach_calc->RuleExists(this->rule_name_);
  }

}
}

