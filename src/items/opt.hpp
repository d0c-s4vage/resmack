#ifndef RESMACK_ITEM_OPT
#define RESMACK_ITEM_OPT

#include "../item.hpp"
#include "../rand.hpp"

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
};

}
}


#endif
