#ifndef RESMACK_RULE_MAN
#define RESMACK_RULE_MAN

#include <string>

#include "types.hpp"
#include "item.hpp"
#include "items/or.hpp"

namespace resmack {

  class RuleManager {
   private:
    Vector<items::Or*> rules_;
    Map<size_t, std::string> rule_idx_to_name_;
    Map<std::string, size_t> rule_name_to_idx_;

   public:
    RuleManager();
    ~RuleManager();

    Vector<items::Or*>* GetRules();
    bool IndexOf(std::string rule_name, size_t* out);
    bool NameOf(size_t rule_idx, std::string* out);
    bool NameExists(std::string rule_name);
    bool IndexExists(size_t rule_idx);
    items::Or* Ensure(std::string rule_name);
    void Prune(std::string rule_name);
    void Prune(size_t rule_idx);
    items::Or* GetRule(std::string rule_name);
    items::Or* GetRule(size_t rule_idx);
  };

}

#endif
