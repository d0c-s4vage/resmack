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
