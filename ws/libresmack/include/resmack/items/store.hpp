#ifndef RESMACK_ITEM_STORE_H
#define RESMACK_ITEM_STORE_H

#include "../item.hpp"

#define STORE(RULE_NAME, VAL) (new resmack::items::Store(RULE_NAME, VAL, false, false))
#define SET(RULE_NAME, VAL) (new resmack::items::Store(RULE_NAME, VAL, true, false))

// invisible storage mechanisms - does not affect the output EXCEPT FOR PRE
// and POST
#define ISTORE(RULE_NAME, VAL) (new resmack::items::Store(RULE_NAME, VAL, false, true))
#define ISET(RULE_NAME, VAL) (new resmack::items::Store(RULE_NAME, VAL, true, true))

namespace resmack {
namespace items {

  // Remember the last value that was generated throug this instance
  class Store: public resmack::Item {
   public:
    Item* item_;
    std::string rule_name;
    size_t rule_idx;
    bool clobber;
    bool invisible;

    Store(std::string rule_name, Item* item, bool clobber, bool invisible);
    ~Store();

    ItemType Type();
    void Build(BuildContext* ctx);
    bool CalcReachability(calc::Reach* reach_calc);
    size_t CalcRefDepth(calc::RefDepth* ref_calc);
  };

}
}

#endif
