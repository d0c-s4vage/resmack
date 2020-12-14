#include "resmack/item.hpp"
#include "resmack/items/post.hpp"

namespace resmack {
namespace items {

  Post::Post(Item* item) : item_(item) {}
  Post::~Post() {
    delete this->item_;
  }

  ItemType Post::Type() {
    return TYPE_POST;
  }

  void Post::Build(BuildContext* ctx) {
    ctx->post_items.push_back(this->item_);
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
