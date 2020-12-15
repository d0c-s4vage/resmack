#include "resmack/item.hpp"
#include "resmack/items/flush.hpp"

namespace resmack {
namespace items {

  ItemType Flush::Type() {
    return TYPE_FLUSHD;
  }

  void Flush::Build(BuildContext* ctx) {
    ctx->FlushPrePost();
  }

  // --------------------------------------------------------------------------

  Flushed::Flushed(Item* item): item_(item) {}
  Flushed::~Flushed() { delete this->item_; }

  ItemType Flushed::Type() {
    return TYPE_FLUSHED;
  }

  void Flushed::Build(BuildContext* ctx) {
    size_t curr_post_item_idx = ctx->post_items.size();

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

    for (; curr_post_item_idx < ctx->post_items.size(); curr_post_item_idx++) {
      ctx->post_items[curr_post_item_idx]->Build(ctx);
    }
    ctx->post_items.erase(
      ctx->post_items.begin() + curr_post_item_idx - 1,
      ctx->post_items.end()
    );
  }

  size_t Flushed::CalcRefDepth(calc::RefDepth* calc) {
    return this->item_->CalcRefDepth(calc);
  }

  bool Flushed::CalcReachability(calc::Reach* calc) {
    return this->item_->CalcReachability(calc);
  }

}
}
