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

}
}
