#ifndef RESMACK_ITEM_CAPTURE_H
#define RESMACK_ITEM_CAPTURE_H

#include "../item.hpp"

#define CAPTURE(VAL) (new resmack::items::Capture(VAL))
#define CAPTURED (new resmack::items::Captured())

namespace resmack {
namespace items {

  // Remember the last value that was generated throug this instance
  class Capture: public resmack::Item {
   public:
    Item* item_;

    Capture(Item* item);
    ~Capture();

    ItemType Type();
    void Build(BuildContext* ctx);
    bool CalcReachability(calc::Reach* reach_calc);
    size_t CalcRefDepth(calc::RefDepth* ref_calc);
  };

  class Captured: public resmack::Item {
   public:
    Captured() {}
    ~Captured() {}

    ItemType Type();
    void Build(BuildContext* ctx);
  };

}
}

#endif
