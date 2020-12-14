#include "resmack/item.hpp"
#include "resmack/items/raw.hpp"

namespace resmack {
namespace items {

  Raw::Raw(std::string data) : data(data) {
  }
  //Raw::Raw(char* data) : data_(data) {}
  //Raw::Raw(const char* data) : data_((char *)data) {}

  ItemType Raw::Type() {
   return TYPE_RAW;
  }

  void Raw::Build(BuildContext *ctx) {
    *ctx->output += this->data;
  }

  std::string Raw::ToString() {
    return std::string("<RAW ") + this->data + ">";
  }

}
}
