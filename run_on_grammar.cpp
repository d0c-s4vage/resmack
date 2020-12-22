#include "resmack/rules.hpp"
#include "resmack/item.hpp"
#include "resmack/items/and.hpp"
#include "resmack/items/or.hpp"
#include "resmack/items/raw.hpp"
#include "resmack/items/ref.hpp"
#include "resmack/items/opt.hpp"

size_t ResmackGrammarInit(resmack::Rules* rules) {
  rules->AddRule("fruit", OR("apples", "bananas", "grapes", "pears", "peaches"))
    ->AddRule("conjunction", OR("or", "and", "with", "without"))
    ->AddRule("fruit-list", AND_S(" ",
      REF("fruit"),
      OPT(AND_S(" ", REF("conjunction"), REF("fruit-list")))
    ))
    ->AddRule("verb", OR(
      "eat", "throw", "stomp on", "enjoy", "purchase", "stare at", "saute",
      "devour", "mock", "ridicule", "praise", "return", "investigate",
      "detest", "abhor", "congratulate"
    ))
    ->AddRule("subject", OR("I", "we", "you", "they"))
    ->AddRule("sentence", AND_S(" ", REF("subject"), REF("verb"), REF("fruit-list")))
    ->AddRule("run-on-sentence", AND(
      REF("sentence"),
      REF("run-on-sentence-opt")      
    ))
    ->AddRule("run-on-sentence-opt", OPT(AND_S(" ", REF("conjunction"), REF("run-on-sentence"))))
    ->AddRule("direct", V("A"));

  size_t rule_idx;
  //if (!rules->GetRuleMan()->IndexOf("direct", &rule_idx)) {
  if (!rules->GetRuleMan()->IndexOf("run-on-sentence", &rule_idx)) {
    std::cout << "Invalid rules" << std::endl;
    std::exit(1);
  }

  return rule_idx;
}
