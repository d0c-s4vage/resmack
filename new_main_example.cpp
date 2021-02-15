#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>

#include "resmack/fuzz/cli/main.hpp"
#include "resmack/fuzz/interface.hpp"

void NonAsanCrash() {
  ((void(*)())(0))();
}

void AsanCrash() {
  char data[10];
  const char* new_data = "THIS IS LONGER THAN THE ARRAY";
  memcpy(data, new_data, strlen(new_data));
}

extern "C"
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  printf("FUZZING IT! pid: %d\n", getpid());
  AsanCrash();
}

int main(int argc, char** argv) {
  resmack::fuzz::cli::Main(argc, argv);
}
