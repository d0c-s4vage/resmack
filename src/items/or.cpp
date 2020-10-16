#include "../item.hpp"
#include "or.hpp"

namespace resmack {
namespace items {

  Or::Or() : Or(false) {}
  Or::Or(bool keep): keep_(keep) {}
  Or::~Or() {
   for (Item *item: this->items_) {
     delete item;
   }
   this->items_.clear();
  }

  ItemType Or::Type() {
   return ItemType::OR;
  }

  void Or::Build(BuildContext* ctx) {
   uint32_t chosen_idx = (ctx->rand->Next() % this->items_.size());
   this->items_[chosen_idx]->Build(ctx);
  }

  Or* Or::AddItem(Item *item) {
   this->items_.push_back(item);
   std::cout << "Adding Item\n";
   return this;
  }

  bool Or::CalcReachability(calc::Reach* reach_calc) {
   this->choice_indices_.clear();

   for (size_t idx = 0; idx < this->items_.size(); idx++) {
     Item* item = this->items_[idx];
     if (item->CalcReachability(reach_calc)) {
       this->choice_indices_.emplace_back(idx);
     }
   }

   // none of our options are reachable!
   return this->choice_indices_.size() == 0;
  }

}
}
