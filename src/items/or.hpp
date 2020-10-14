#ifndef RESMACK_ITEM_OR
#define RESMACK_ITEM_OR

#include <vector>
#include <iostream>

#include "../item.hpp"
#include "../rand.hpp"

namespace resmack {
namespace items {

class Or: public resmack::Item {
  private:
   std::vector<resmack::Item*> items_;

  public:
   Or() : items_() {
     std::cout << "Created OR\n";
   }

   ~Or() {
     for (Item *item: this->items_) {
       delete item;
     }
     this->items_.clear();
   }

   ItemType Type() {
     return ItemType::OR;
   }

   void Build(BuildContext* ctx) {
     uint32_t chosen_idx = (ctx->rand->Next() % this->items_.size());
     this->items_[chosen_idx]->Build(ctx);
   }

   Or* AddItem(Item *item) {
     this->items_.push_back(item);
     std::cout << "Adding Item\n";
     return this;
   }
};

}
}

#endif
