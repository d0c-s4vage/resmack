#include "resmack/rules.hpp"
#include "resmack/item.hpp"
#include "resmack/build_context.hpp"
#include "resmack/items/scope.hpp"

namespace resmack {
namespace items {

  Scope::Scope(Item* item): item_(item) {}
  Scope::~Scope() {
    delete this->item_;
  }

  ItemType Scope::Type() {
    return ItemType::SCOPE;
  }

  void Scope::Build(BuildContext *ctx) {
    Rules* orig_rules = ctx->rules;
    Rules* tmp_rules = ctx->rules->NewChild();
    ctx->rules = tmp_rules;
    this->item_->Build(ctx);
    // use a stack var?
    delete tmp_rules;
    ctx->rules = orig_rules;
  }

  bool Scope::CalcReachability(calc::Reach* reach_calc) {
    return this->item_->CalcReachability(reach_calc);
  }

  size_t Scope::CalcRefDepth(calc::RefDepth* ref_depth) {
    return this->item_->CalcRefDepth(ref_depth);
  }

  std::string Scope::ToString() {
    return std::string("<SCOPE ") + this->item_->ToString() + ">";
  }

}
}
