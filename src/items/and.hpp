#ifndef RESMACK_ITEM_AND
#define RESMACK_ITEM_AND

#include <stdint.h>
#include <vector>
#include <iostream>

#include "../item.hpp"
#include "../rand.hpp"

namespace resmack {
namespace items {

class And: public resmack::Item {
  private:
   std::vector<resmack::Item*> items_;
   std::string sep_;

  public:
   And(std::string sep) : items_(), sep_(sep) {}
   And() : items_(), sep_("") {}

   ~And() {
     for (Item *item: this->items_) {
       delete item;
     }
     this->items_.clear();
   }

   ItemType Type() {
     return ItemType::AND;
   }

   void Build(BuildContext *ctx) {
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

   And* AddItem(Item *item) {
     this->items_.push_back(item);
     std::cout << "Adding Item\n";
     return this;
   }
};

}
}

#endif
