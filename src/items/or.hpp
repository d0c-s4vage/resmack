#ifndef RESMACK_ITEM_OR
#define RESMACK_ITEM_OR

#include <vector>
#include <iostream>

#include "../item.hpp"
#include "../rand.hpp"
#include "build_context.hpp"

namespace resmack {

namespace calc {
  class Reach;
  class RefDepth;
}

namespace items {

  class Or: public resmack::Item {
   private:
    std::vector<resmack::Item*> items_;
    std::vector<size_t> choice_indices_;
    std::vector<size_t> shortest_indices_;
    bool keep_;

   public:
    Or();
    Or(bool keep);
    ~Or();

    ItemType Type();
    void Build(BuildContext* ctx);
    Or* AddItem(Item *item);
    /**
     * Add a variable number of items to this Or. NULL *MUST* be passed
     * as the final argument as a sentinel value
     **/
    Or* AddItems(Item* item, .../*Item* item, .., NULL sentinel value*/);
    size_t NumItems() { return this->items_.size(); }
    size_t NumChoicesItems() { return this->choice_indices_.size(); }
    size_t NumShortestItems() { return this->shortest_indices_.size(); }
    bool ShouldKeep() { return this->keep_; }

    bool CalcReachability(calc::Reach* reach_calc);
    size_t CalcRefDepth(calc::RefDepth* ref_calc);
  };

}
}

#endif
