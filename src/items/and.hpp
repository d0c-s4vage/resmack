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
  And(std::string sep);
  And();
  ~And();

  ItemType Type();
  void Build(BuildContext *ctx);
  And* AddItem(Item *item);
};

}
}

#endif
