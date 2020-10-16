#include <ctime>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "rules.hpp"
#include "items/or.hpp"
#include "items/and.hpp"
#include "items/str.hpp"
#include "items/ref.hpp"
#include "items/opt.hpp"
#include "items/raw.hpp"
#include "rand.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace resmack;

int main(int argc, char** argv) {
  Rules rules;

  Rand rand;
  std::string output;
  output.reserve(0x100);

  items::Raw str1("Hello World1");
  items::Raw str2("Hello World2");
  items::Raw str3("Hello World3");
  items::And and_("<->");
  and_.AddItem(new items::Raw("And Item 1"));
  and_.AddItem(new items::Raw("And Item 2"));
  and_.AddItem(new items::Raw("And Item 3"));
  and_.AddItem(new items::Raw("And Item 4"));

  rules.AddRule("test_rule", &str1);
  rules.AddRule("test_rule", &str2);
  rules.AddRule("test_rule", &str3);
  rules.AddRule("test_rule", new items::Str(10, 20));
  rules.AddRule("test_rule", &and_);

  items::Ref ref("test_rule");
  items::And big_and("==");
  big_and\
    .AddItem(new items::Raw("hello"))\
    ->AddItem((new items::Or())\
        ->AddItem(new items::Raw("blah"))\
        ->AddItem(new items::Raw("halb"))\
        ->AddItem(new items::Opt(new items::Raw("testing")))
    );

  rules.AddRule("other_rule", &ref);
  rules.AddRule("other_rule", &big_and);

  uint64_t total_bytes = 0;
  uint64_t count = 0;
  float start = clock() / (float)CLOCKS_PER_SEC;

  while (true) {
    count += 1;

    output.clear();
    rules.Build("other_rule", &output, &rand);
    total_bytes += output.size();

    if (count % 0x100000 == 0) {
      float curr = clock() / (float)CLOCKS_PER_SEC;
      float totalMibs = (float)total_bytes / (1024.0f * 1024.0f);
      printf("%08lx | %0.2f iters/s | %0.2f MiB/s\n",
             count,
             (float)count / (curr - start),
             totalMibs / (curr - start));
    }
  }
}
