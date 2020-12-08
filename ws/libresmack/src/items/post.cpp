#include "resmack/item.hpp"
#include "resmack/items/post.hpp"

namespace resmack {
namespace items {

  Post::Post(Item* item) : item_(item) {}
  Post::~Post() {
    delete this->item_;
  }

  ItemType Post::Type() {
    return ItemType::POST;
  }

  void Post::Build(BuildContext* ctx) {
    std::string tmp_post_output;
    std::string* orig_output = ctx->output;

    ctx->output = ctx->post_output;
    ctx->post_output = &tmp_post_output;

    this->item_->Build(ctx);

    ctx->post_output = ctx->output;
    ctx->output = orig_output;

    (*ctx->post_output) += tmp_post_output;
  }

  bool Post::CalcReachability(calc::Reach* reach_calc) {
    return this->item_->CalcReachability(reach_calc);
  }

  size_t Post::CalcRefDepth(calc::RefDepth* ref_calc) {
    return this->item_->CalcRefDepth(ref_calc);
  }

  std::string Post::ToString() {
    return std::string("<Post ") + this->item_->ToString() + ">";
  }

}
}
