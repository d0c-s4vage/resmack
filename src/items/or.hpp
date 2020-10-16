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
}

namespace items {

class Or: public resmack::Item {
 private:
  std::vector<resmack::Item*> items_;
  std::vector<size_t> choice_indices_;
  bool keep_;

 public:
  Or();
  Or(bool keep);
  ~Or();

  ItemType Type();
  void Build(BuildContext* ctx);
  Or* AddItem(Item *item);
  bool CalcReachability(calc::Reach* reach_calc);
};

}
}

#endif
