#ifndef RESMACK_ITEM
#define RESMACK_ITEM

#define OVERLOADED_MACRO(M, ...) _OVR(M, _COUNT_ARGS(__VA_ARGS__)) (__VA_ARGS__)
#define _OVR(macroName, number_of_args)   _OVR_EXPAND(macroName, number_of_args)
#define _OVR_EXPAND(macroName, number_of_args)    macroName##number_of_args

#define _COUNT_ARGS(...)  _ARG_PATTERN_MATCH(__VA_ARGS__, 9,8,7,6,5,4,3,2,1)
#define _ARG_PATTERN_MATCH(_1,_2,_3,_4,_5,_6,_7,_8,_9, N, ...)   N

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

  static std::string ItemTypeName(ItemType type) {
    switch (type) {
      case ItemType::RAW:
        return "RAW";
        break;
      case ItemType::STR:
        return "STR";
        break;
      case ItemType::INT:
        return "INT";
        break;
      case ItemType::OR:
        return "OR";
        break;
      case ItemType::AND:
        return "AND";
        break;
      case ItemType::REF:
        return "REF";
        break;
      case ItemType::OPT:
        return "OPT";
        break;
    };
    return "??";
  }

  class Item {
    public:
     virtual ~Item() { };
     virtual ItemType Type() = 0;
     virtual void Build(BuildContext *ctx) = 0;
     virtual std::string ToString() {
       return std::string("<") + ItemTypeName(this->Type()) + ">";
     }
     virtual bool CalcReachability(calc::Reach *calc __attribute__((unused))) {
       return true;
     }
     virtual size_t CalcRefDepth(calc::RefDepth *calc __attribute__((unused))) {
       return 0;
     }
  };

}

#endif
