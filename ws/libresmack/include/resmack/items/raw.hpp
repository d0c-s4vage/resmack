#ifndef RESMACK_ITEM_RAW
#define RESMACK_ITEM_RAW

#define RAW(VAL) (new resmack::items::Raw(VAL))
#define V(VAL) RAW(VAL)

#include <cstdlib>
#include <iostream>

#include "../item.hpp"

namespace resmack {
namespace items {

  class Raw: public resmack::Item {
   private:
     std::string data;

   public:
    //Raw(char* data);
    //Raw(const char* data);
    Raw(std::string data);
    ~Raw();

    ItemType Type();
    void Build(BuildContext* ctx);
    std::string ToString();
  };

}
}

#endif
