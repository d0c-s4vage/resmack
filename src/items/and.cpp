#include "../item.hpp"
#include "and.hpp"

namespace resmack {
namespace items {

  And::And(std::string sep) : items_(), sep_(sep) {}
  And::And() : items_(), sep_("") {}

  And::~And() {
   for (Item *item: this->items_) {
     delete item;
   }
   this->items_.clear();
  }

  ItemType And::Type() {
   return ItemType::AND;
  }

  void And::Build(BuildContext *ctx) {
   uint32_t count = 0;
   bool add_sep = (this->sep_.size() > 0);
   for (Item* item: this->items_) {
      item->Build(ctx);
      count += 1;
      if (add_sep && count++ > 0 && count <= this->items_.size()) {
        *ctx->output += this->sep_;
      }
   }
  }

  And* And::AddItem(Item *item) {
   this->items_.push_back(item);
   return this;
  }

  bool And::CalcReachability(calc::Reach* reach_calc) {
    for (Item *item: this->items_) {
      if (!item->CalcReachability(reach_calc)) {
        return false;
      }
    }

    return true;
  }

  /**
   * Returns the maximum reference depth of all items in this And
   */
  size_t And::CalcRefDepth(calc::RefDepth* ref_depth) {
    size_t max_len = 0;

    for (Item *item: this->items_) {
      size_t len = item->CalcRefDepth(ref_depth);
      if (len > max_len) {
        max_len = len;
      }
    }

    return max_len;
  }

}
}
