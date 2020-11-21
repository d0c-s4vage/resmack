#include <stdint.h>
#include "ws/libresmack_fuzz/include/resmack/fuzz/interface.hpp"
#include <sys/types.h>
#include <bits/stdint-uintn.h>
#include <sys/types.h>
#include <iostream>
#include <vector>
#include <set>
#include <string>

#include "resmack/rules.hpp"
#include "resmack/item.hpp"
#include "resmack/items/and.hpp"
#include "resmack/items/or.hpp"
#include "resmack/items/raw.hpp"
#include "resmack/items/ref.hpp"
#include "resmack/items/opt.hpp"

void splitStr(std::string* input, std::string split, std::vector<std::string>* output) {
  size_t last_idx = 0;
  size_t split_idx = input->find(split, last_idx);
  while (split_idx != std::string::npos) {
    output->emplace_back(input->substr(last_idx, split_idx - last_idx));
    last_idx = split_idx + split.size();
    split_idx = input->find(split, last_idx);
  }
  if (last_idx < input->size() - 1) {
    output->emplace_back(input->substr(last_idx, input->size() - last_idx));
  }
}

bool parseFruitList(std::vector<std::string>* parts, size_t* curr_idx) {
  std::vector<std::string> fruits;
  std::string* next_part;
  bool expect_fruit = false;

  while (*curr_idx < parts->size()) {
    if (fruits.size() == 4) {
      break;
    }
    expect_fruit = !expect_fruit;
    next_part = &(*parts)[(*curr_idx)++];
    if (expect_fruit) {
      fruits.emplace_back(*next_part);
    } else {
      // only or is allowed
      if (*next_part == "or") {
        continue;
      } else {
        return false;
      }
    }
  }

  if (fruits.size() == 4 && fruits[0] == "apples" && fruits[1] == "grapes" && fruits[2] != "pears") {
    return true;
  }
  return false;
}

bool parseVerb(std::vector<std::string>* parts, size_t* curr_idx) {
  std::string* next_part = &(*parts)[(*curr_idx)++];
  if (*next_part == "throw") {
    return true;
  }
  return false;
}

bool parseSubject(std::vector<std::string>* parts, size_t* curr_idx) {
  std::string* next_part = &(*parts)[(*curr_idx)++];
  if (*next_part == "I") {
    return true;
  }
  return false;
}

bool parseSentence(const uint8_t* data, size_t size) {
  std::string input;
  input.assign((const char*)data, size);
  std::vector<std::string> parts;
  splitStr(&input, " ", &parts);
  size_t curr_idx = 0;

  if (!parseSubject(&parts, &curr_idx)) {
    return false;
  }
  if (!parseVerb(&parts, &curr_idx)) {
    return false;
  }
  if (!parseFruitList(&parts, &curr_idx)) {
    return false;
  }
  if (parts.size() - curr_idx != 4) {
    return false;
  }
  if (parts[curr_idx++] == "and" && parts[curr_idx++] == "we" && parts[curr_idx++] == "devour" && parts[curr_idx++] == "pears") {
    *((char *)1-1) = 'a';
    return true;
  }

  return false;
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  return parseSentence(data, size) ? 1 : 0;
}

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
    ->AddRule("subject", OR("I", "we", "you"))
    ->AddRule("sentence", AND_S(" ", REF("subject"), REF("verb"), REF("fruit-list")))
    ->AddRule("run-on-sentence", AND(
      REF("sentence"),
      OPT(AND_S(" ", REF("conjunction"), REF("run-on-sentence")))
    ));

  size_t rule_idx;
  if (!rules->GetRuleMan()->IndexOf("run-on-sentence", &rule_idx)) {
    std::cout << "Invalid rules" << std::endl;
    std::exit(1);
  }

  return rule_idx;
}
