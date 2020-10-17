#ifndef RESMACK_ITEM_INT
#define RESMACK_ITEM_INT

#include <cstdlib>
#include <iostream>

#include "../item.hpp"

namespace resmack {
namespace items {

class Int: public resmack::Item {
 private:
  int64_t min_;
  int64_t max_;

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
