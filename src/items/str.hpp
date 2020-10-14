#ifndef RESMACK_ITEM_STR
#define RESMACK_ITEM_STR

#include <cstdlib>
#include <iostream>

#include "../item.hpp"

namespace resmack {
namespace items {

class Str: public resmack::Item {
  private:
   char* data_;

  public:
   Str(char* data) : data_(data) {
   }

   Str(const char* data) : data_((char *)data) {
   }

   ~Str() {
   }

   ItemType Type() {
     return ItemType::STR;
   }

   void Build(BuildContext *ctx) {
     ctx->output->append(this->data_);
   }
};

}
}

#endif
