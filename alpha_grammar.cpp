#include "resmack/rules.hpp"
#include "resmack/item.hpp"
#include "resmack/items/str.hpp"
#include "resmack/items/ref.hpp"
#include "resmack/items/and.hpp"
#include "resmack/items/opt.hpp"

size_t ResmackGrammarInit(resmack::Rules* rules) {
  std::cout << "USING ALPHA GRAMMAR" << std::endl;

  rules->AddRule("word", STR(1, 10, "abcdefghijklmnopqrstuvwxyzI"))
    ->AddRule("sentence", AND(
      REF("word"),
      OPT(AND(V(" "), REF("sentence")))
    ));

  size_t rule_idx;
  if (!rules->GetRuleMan()->IndexOf("sentence", &rule_idx)) {
    std::cout << "Invalid rules" << std::endl;
    std::exit(1);
  }

  return rule_idx;
}
