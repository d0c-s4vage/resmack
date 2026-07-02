#include "gtest/gtest.h"

#include "resmack/rules.hpp"
#include "resmack/item.hpp"
#include "resmack/rand.hpp"
#include "resmack/items/str.hpp"

#include "test_utils.hpp"

using namespace std::string_literals;

namespace resmack {
namespace items {

  class CustomLenInserted : public Item {
   public:
    Item* item_;

    CustomLenInserted(Item* item) : item_(item) {}
    ~CustomLenInserted() {
      delete this->item_;
    }

    ItemType Type() {
      return TYPE_CUSTOM;
    }

    void Build(BuildContext *ctx) {
      size_t len;
      len = 0;

      size_t len_pos = ctx->output->size();
      *ctx->output += static_cast<char>(len);

      this->item_->Build(ctx);

      len = ctx->output->size() - len_pos - sizeof(len);
      ctx->output->data()[len_pos] = static_cast<char>(len);
    }
  };

  TEST(Custom, CustomItemsWork)
  {
    Rand rand(100);
    CustomLenInserted* custom = new CustomLenInserted(STR(0x41, 0x42, "B", 1));

    std::string built = test_utils::BuildItem(custom);
    EXPECT_EQ(built, "ABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
  }

}
}
