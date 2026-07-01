#include "resmack/debug.hpp"
#include "resmack/item.hpp"
#include "resmack/items/flush.hpp"

namespace resmack {
namespace items {

  ItemType Flush::Type() {
    return TYPE_FLUSHD;
  }

  void Flush::Build(BuildContext* ctx) {
    DEBUG_PRINT("Flush::Build calling FlushPrePost: %p\n", ctx->pre_output);
    ctx->FlushPrePost();
  }

  // --------------------------------------------------------------------------

  Flushed::Flushed(Item* item): item_(item) {}
  Flushed::~Flushed() { delete this->item_; }

  ItemType Flushed::Type() {
    return TYPE_FLUSHED;
  }

  void Flushed::Build(BuildContext* ctx) {
    size_t next_item_idx = ctx->post_items.size();

    std::string tmp_output;
    std::string tmp_pre_output;

    std::string* orig_pre_output = ctx->pre_output;
    std::string* orig_output = ctx->output;

    ctx->pre_output = &tmp_pre_output;
    ctx->output = &tmp_output;

    this->item_->Build(ctx);

    ctx->output = orig_output;
    ctx->pre_output = orig_pre_output;

    *ctx->output += tmp_pre_output;
    *ctx->output += tmp_output;

    for (size_t idx = next_item_idx; idx < ctx->post_items.size(); idx++) {
      ctx->post_items[idx]->Build(ctx);
    }
    ctx->post_items.resize(next_item_idx);
  }

  size_t Flushed::CalcRefDepth(calc::RefDepth* calc) {
    return this->item_->CalcRefDepth(calc);
  }

  bool Flushed::CalcReachability(calc::Reach* calc) {
    return this->item_->CalcReachability(calc);
  }

}
}
