#ifndef RESMACK_ITEM
#define RESMACK_ITEM

#define OVERLOADED_MACRO(M, ...) OVR(M, COUNT_ARGS(__VA_ARGS__)) (__VA_ARGS__)
#define OVR(macroName, number_of_args)   OVR_EXPAND(macroName, number_of_args)
#define OVR_EXPAND(macroName, number_of_args)    macroName##number_of_args

#define COUNT_ARGS(...)  ARG_PATTERN_MATCH(__VA_ARGS__, 9,8,7,6,5,4,3,2,1)
#define ARG_PATTERN_MATCH(_1,_2,_3,_4,_5,_6,_7,_8,_9, N, ...)   N

#include <string>

#include "resmack/defs.hpp"

#include "rand.hpp"
#include "build_context.hpp"

namespace resmack {

  namespace calc {
    class Reach;
    class RefDepth;
  }

  class Rules;

  enum ItemType {
    TYPE_RAW,
    TYPE_STR,
    TYPE_INT,
    TYPE_OR,
    TYPE_AND,
    TYPE_REF,
    TYPE_OPT,
    TYPE_ID,
    TYPE_SCOPE,
    TYPE_SCOPE_ACTION,
    TYPE_PRE,
    TYPE_POST,
    TYPE_CAPTURE,
    TYPE_CAPTURED,
    TYPE_FLUSHD,
    TYPE_FLUSHED,
    TYPE_CUSTOM,
  };

  static std::string ItemTypeName(ItemType type) {
    switch (type) {
      case TYPE_RAW:
        return "RAW";
        break;
      case TYPE_STR:
        return "STR";
        break;
      case TYPE_INT:
        return "INT";
        break;
      case TYPE_OR:
        return "OR";
        break;
      case TYPE_AND:
        return "AND";
        break;
      case TYPE_REF:
        return "REF";
        break;
      case TYPE_OPT:
        return "OPT";
        break;
      case TYPE_ID:
        return "ID";
        break;
      case TYPE_SCOPE:
        return "SCOPE";
        break;
      case TYPE_PRE:
        return "PRE";
        break;
      case TYPE_POST:
        return "POST";
        break;
      case TYPE_CAPTURE:
        return "CAPTURE";
        break;
      case TYPE_CAPTURED:
        return "CAPTURED";
        break;
      case TYPE_FLUSHD:
        return "FLUSH";
        break;
      case TYPE_FLUSHED:
        return "FLUSHED";
        break;
      case TYPE_SCOPE_ACTION:
        return "SCOPE_ACTION";
        break;
      case TYPE_CUSTOM:
        return "CUSTOM";
        break;
    };
    return "??";
  }

  class Item {
    public:
     virtual ~Item() { };
     virtual ItemType Type() = 0;
     virtual void Build(BuildContext* ctx) = 0;
     virtual bool IntendsOutput() { return true; }
     virtual std::string ToString() {
       return std::string("<") + ItemTypeName(this->Type()) + ">";
     }
     virtual bool CalcReachability(calc::Reach *calc) {
       UNUSED(calc);
       return true;
     }
     virtual size_t CalcRefDepth(calc::RefDepth *calc) {
       UNUSED(calc);
       return 0;
     }
  };

}

#endif
