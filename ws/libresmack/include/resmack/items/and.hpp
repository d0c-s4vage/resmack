#ifndef RESMACK_ITEM_AND
#define RESMACK_ITEM_AND

#define AND(...) ((new resmack::items::And())->AddItems(__VA_ARGS__, NULL))
#define AND_S(SEP, ...) ((new resmack::items::And(SEP))->AddItems(__VA_ARGS__, NULL))

#include <string>
#include <stdint.h>
#include <vector>
#include <iostream>

#include "../item.hpp"
#include "../rand.hpp"

namespace resmack {

namespace calc {
  class Reach;
  class RefDepth;
}

namespace items {

  class And: public resmack::Item {
   private:
    std::vector<resmack::Item*> items_;
    std::string sep_;

   public:
    And(std::string sep);
    And();
    ~And();

    ItemType Type();
    void Build(BuildContext* ctx);
    And* AddItem(Item *item);
    /**
     * Add a variable number of items to this Or. NULL *MUST* be passed
     * as the final argument as a sentinel value
     **/
    And* AddItems(Item* item, .../*Item* item, .., NULL sentinel value*/);
    And* AddItems(const char* item, .../*Item* item, ..., NULL sentinel value*/);
    bool CalcReachability(calc::Reach* reach_calc);
    size_t CalcRefDepth(calc::RefDepth* ref_depth);
    std::string ToString();
  };

}
}

#endif
