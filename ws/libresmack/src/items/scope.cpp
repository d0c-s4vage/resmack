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
    return TYPE_SCOPE;
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

  // -------------------------------------------------------------------------

  ScopeAction::ScopeAction(bool push): push(push) {}
  ScopeAction::~ScopeAction() {}

  ItemType ScopeAction::Type() { return TYPE_SCOPE_ACTION; }

  void ScopeAction::Build(BuildContext* ctx) {
    if (this->push) {
      ctx->rules = ctx->rules->GetParent()->NewChild();
    } else {
      Rules* child_rules = ctx->rules;
      ctx->rules = child_rules->GetParent();
      delete child_rules;
    }
  }

}
}
