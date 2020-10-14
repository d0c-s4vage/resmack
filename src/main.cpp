#include <ctime>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "rules.hpp"
#include "items/or.hpp"
#include "items/and.hpp"
#include "items/str.hpp"
#include "items/ref.hpp"
#include "rand.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace resmack;

int main(int argc, char** argv) {
  Rules rules;

  Rand rand;
  std::string output;
  output.reserve(0x100);

  items::Str str1("Hello World1");
  items::Str str2("Hello World2");
  items::Str str3("Hello World3");
  items::And and_("<->");
  and_.AddItem((Item*)new items::Str("And Item 1"));
  and_.AddItem((Item*)new items::Str("And Item 2"));
  and_.AddItem((Item*)new items::Str("And Item 3"));
  and_.AddItem((Item*)new items::Str("And Item 4"));

  rules.AddRule("test_rule", (Item*)&str1);
  rules.AddRule("test_rule", (Item*)&str2);
  rules.AddRule("test_rule", (Item*)&str3);
  rules.AddRule("test_rule", (Item*)&and_);

  items::Ref ref("test_rule");
  items::And big_and("==");
  big_and\
    .AddItem(new items::Str("hello"))\
    ->AddItem((new items::Or())\
        ->AddItem(new items::Str("blah"))\
        ->AddItem(new items::Str("halb"))
    );

  rules.AddRule("other_rule", (Item*)&ref);
  rules.AddRule("other_rule", (Item*)&big_and);

  uint64_t total_bytes = 0;
  uint64_t count = 0;
  float start = clock() / (float)CLOCKS_PER_SEC;

  while (true) {
    count += 1;

    output.clear();
    rules.Build("other_rule", &output, &rand);
    total_bytes += output.size();

    if (count % 0x1000000 == 0) {
      float curr = clock() / (float)CLOCKS_PER_SEC;
      float totalMibs = (float)total_bytes / (1024.0f * 1024.0f);
      //printf("output: %s\n", output.c_str());
      printf("%08lx | %0.2f iters/s | %0.2f MiB/s\n",
             count,
             (float)count / (curr - start),
             totalMibs / (curr - start));
    }
  }
}
