#include <stdint.h>
#include "ws/libresmack_fuzz/include/resmack/fuzz/interface.hpp"
#include <sys/types.h>
#include <bits/stdint-uintn.h>
#include <sys/types.h>
#include <signal.h>
#include <iostream>
#include <vector>
#include <set>
#include <string>

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

  if (parts.size() == 0) {
    return false;
  }

  if (curr_idx >= parts.size() || !parseSubject(&parts, &curr_idx)) {
    return false;
  }
  if (curr_idx >= parts.size() || !parseVerb(&parts, &curr_idx)) {
    return false;
  }
  if (curr_idx >= parts.size() || !parseFruitList(&parts, &curr_idx)) {
    return false;
  }
  if (parts.size() - curr_idx != 4) {
    return false;
  }
  if (parts[curr_idx++] == "and" && parts[curr_idx++] == "we" && parts[curr_idx++] == "devour" && parts[curr_idx++] == "pears") {
    raise(SIGSEGV);
  }

  return false;
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  return parseSentence(data, size) ? 1 : 0;
}

