#ifndef RESMACK_SCOPE
#define RESMACK_SCOPE

#include "../item.hpp"

#define SCOPE(ITEM) (new resmack::items::Scope(ITEM))

namespace resmack {
namespace items {

  class Scope: public resmack::Item {
   public:
    Item* item_;

    Scope(Item* item);
    ~Scope();

    ItemType Type();
    void Build(BuildContext *ctx);
    bool CalcReachability(calc::Reach* reach_calc);
    size_t CalcRefDepth(calc::RefDepth* ref_calc);
    std::string ToString();
  };

}
}

#endif
