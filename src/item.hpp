#ifndef RESMACK_ITEM
#define RESMACK_ITEM

#include <string>

#include "rand.hpp"
#include "build_context.hpp"

namespace resmack {

enum ItemType {
  STR,
  INT,
  OR,
  AND,
  REF,
};

class Item {
  public:
   virtual ~Item() { };
   virtual ItemType Type() =0;
   virtual void Build(BuildContext *ctx)= 0;
};

}

#endif
