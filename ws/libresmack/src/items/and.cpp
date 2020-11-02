#include <cstdarg>

#include "resmack/item.hpp"
#include "resmack/items/raw.hpp"
#include "resmack/items/and.hpp"

namespace resmack {
namespace items {

  And::And(std::string sep) : items_(), sep_(sep) {}
  And::And() : items_(), sep_("") {}

  And::~And() {
   for (Item *item: this->items_) {
     delete item;
   }
   this->items_.clear();
  }

  ItemType And::Type() {
   return ItemType::AND;
  }

  void And::Build(BuildContext *ctx) {
   uint32_t count = 0;
   bool add_sep = (this->sep_.size() > 0);
   for (Item* item: this->items_) {
      if (add_sep && count > 0 && count <= this->items_.size()) {
        *ctx->output += this->sep_;
      }
      count++;
      item->Build(ctx);
   }
  }

  And* And::AddItem(Item *item) {
   this->items_.push_back(item);
   return this;
  }

  And* And::AddItems(Item* first, .../*Item* item, .., NULL sentinel value*/) {
    va_list args;
    va_start(args, first);
    Item* next = first;

    while (NULL != next) {
      if (next == NULL) { break; }
      this->items_.push_back(next);
      next = va_arg(args, Item*);
    }
    return this;
  }

  And* And::AddItems(const char* first, .../*std:;string item, .., NULL sentinel value*/) {
    va_list args;
    va_start(args, first);
    const char* next = first;

    while (NULL != next) {
      if (next == NULL) { break; }
      this->items_.push_back(new items::Raw(std::string(next)));
      next = va_arg(args, const char*);
    }
    return this;
  }

  bool And::CalcReachability(calc::Reach* reach_calc) {
    for (Item *item: this->items_) {
      if (!item->CalcReachability(reach_calc)) {
        return false;
      }
    }

    return true;
  }

  /**
   * Returns the maximum reference depth of all items in this And
   */
  size_t And::CalcRefDepth(calc::RefDepth* ref_depth) {
    size_t max_len = 0;

    for (Item *item: this->items_) {
      size_t len = item->CalcRefDepth(ref_depth);
      if (len > max_len) {
        max_len = len;
      }
    }

    return max_len;
  }

  std::string And::ToString() {
    std::string res = "<AND";

    res += " items=[";
    for (auto item: this->items_) {
      res += item->ToString() + ",";
    }
    if (this->items_.size() > 0) {
      res[res.size()-1] = ']';
    } else {
      res += "]";
    }

    return res + ">";
  }

}
}
