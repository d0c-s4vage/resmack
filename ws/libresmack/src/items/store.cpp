#include "resmack/rules.hpp"
#include "resmack/items/store.hpp"
#include "resmack/items/raw.hpp"

#include "calc/reach.hpp"

namespace resmack {
namespace items {

  Store::Store(std::string rule_name, Item* item, bool clobber, bool invisible):
    item_(item),
    rule_name(rule_name),
    clobber(clobber),
    invisible(invisible)
  {}
  Store::~Store() { delete this->item_; }

  ItemType Store::Type() { return TYPE_CAPTURE; }

  bool Store::CalcReachability(calc::Reach* calc) {
    calc->Ensure(this->rule_name, &this->rule_idx);
    return this->item_->CalcReachability(calc);
  }

  size_t Store::CalcRefDepth(calc::RefDepth* calc) {
    return this->item_->CalcRefDepth(calc);
  }

  void Store::Build(BuildContext* ctx) {
    size_t start_pos = ctx->output->size();
    this->item_->Build(ctx);
    size_t len = ctx->output->size() - start_pos;

    if (len == 0) { return; }

    if (this->clobber) {
      ctx->rules->GetRuleMan()->ClearRuleValues(this->rule_idx);
    }

    ctx->rules->AddRule(
      this->rule_idx,
      new Raw(ctx->output->substr(start_pos, len))
    );

    if (this->invisible) {
      ctx->output->resize(start_pos);
    }
  }

}
}
