#include "resmack/item.hpp"
#include "resmack/items/pre.hpp"

namespace resmack {
namespace items {

  Pre::Pre(Item* item) : item_(item) {}
  Pre::~Pre() {}

  ItemType Pre::Type() {
    return ItemType::PRE;
  }

  void Pre::Build(BuildContext *ctx) {
    std::string tmp_pre_output;
    std::string *orig_output = ctx->output;

    ctx->output = ctx->pre_output;
    ctx->pre_output = &tmp_pre_output;

    this->item_->Build(ctx);

    ctx->pre_output = ctx->output;
    ctx->output = orig_output;

    (*ctx->pre_output) = tmp_pre_output + *ctx->pre_output;
  }

  bool Pre::CalcReachability(calc::Reach* reach_calc) {
    return this->item_->CalcReachability(reach_calc);
  }

  size_t Pre::CalcRefDepth(calc::RefDepth* ref_calc) {
    return this->item_->CalcRefDepth(ref_calc);
  }

  std::string Pre::ToString() {
    return std::string("<Pre ") + this->item_->ToString() + ">";
  }

}
}
