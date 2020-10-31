#include "resmack/item.hpp"
#include "resmack/items/ref.hpp"
#include "resmack/rules.hpp"

#include "calc/reach.hpp"
#include "calc/ref_depth.hpp"

namespace resmack {
namespace items {

  Ref::Ref(std::string rule_name) : rule_name_(rule_name) {}
  Ref::~Ref() {}

  ItemType Ref::Type() {
    return ItemType::REF;
  }

  void Ref::Build(BuildContext *ctx) {
    ctx->IncDepth();
    ctx->rules->Build(this->rule_idx_, ctx);
    ctx->DecDepth();
  }

  bool Ref::CalcReachability(calc::Reach* reach_calc) {
    return reach_calc->IndexOf(this->rule_name_, &this->rule_idx_);
  }

  size_t Ref::CalcRefDepth(calc::RefDepth* ref_calc) {
    size_t res = ref_calc->DepthOf(this->rule_name_);
    if (res == calc::RefDepth::INF_DEPTH) {
      return res;
    }
    return res + 1;
  }

  std::string Ref::ToString() {
    return std::string("<REF ")
      + this->rule_name_
      + " (" + std::to_string(this->rule_idx_)
      + ")>";
  }

}
}
