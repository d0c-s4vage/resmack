#ifndef RESMACK_ITEM_OPT
#define RESMACK_ITEM_OPT

#define OPT(ITEM) (new resmack::items::Opt(ITEM))

#include "resmack/item.hpp"
#include "resmack/rand.hpp"

#include "../item.hpp"
#include "../build_context.hpp"

namespace resmack {
namespace items {

class Opt: public resmack::Item {
 private:
  resmack::Item* item_;

 public:
  Opt(resmack::Item* item);
  ~Opt();

  ItemType Type();
  void Build(BuildContext* ctx);
  bool CalcReachability(calc::Reach* reach_calc);
  size_t CalcRefDepth(calc::RefDepth* calc);
};

}
}


#endif
