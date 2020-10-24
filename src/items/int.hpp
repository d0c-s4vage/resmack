#ifndef RESMACK_ITEM_INT
#define RESMACK_ITEM_INT

#define INT(MIN, MAX) (new resmack::items::Int(MIN, MAX))

#include <cstdlib>
#include <iostream>

#include "item.hpp"

namespace resmack {
namespace items {

class Int: public resmack::Item {
 private:
  int64_t min_;
  int64_t max_;
  int64_t range_;

 public:
  Int();
  Int(int64_t min, int64_t max);
  ~Int();

  ItemType Type();
  void Build(BuildContext *ctx);
};

}
}

#endif
