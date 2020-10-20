#ifndef RESMACK_ITEM
#define RESMACK_ITEM

#include <string>

#include "rand.hpp"
#include "build_context.hpp"

namespace resmack {

  namespace calc {
    class Reach;
    class RefDepth;
  }

  class Rules;

  enum ItemType {
    RAW,
    STR,
    INT,
    OR,
    AND,
    REF,
    OPT,
  };

  class Item {
    public:
     virtual ~Item() { };
     virtual ItemType Type() = 0;
     virtual void Build(BuildContext *ctx) = 0;
     virtual bool CalcReachability(calc::Reach *calc __attribute__((unused))) {
       return true;
     }
     virtual size_t CalcRefDepth(calc::RefDepth *calc __attribute__((unused))) {
       return 0;
     }
  };

}

#endif
