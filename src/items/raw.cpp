#include "../item.hpp"
#include "raw.hpp"

namespace resmack {
namespace items {

  Raw::Raw(char* data) : data_(data) {
  }

  Raw::Raw(const char* data) : data_((char *)data) {
  }

  Raw::~Raw() {
  }

  ItemType Raw::Type() {
   return ItemType::RAW;
  }

  void Raw::Build(BuildContext *ctx) {
   ctx->output->append(this->data_);
  }

}
}
