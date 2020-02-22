#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

uint64_t dataset[] = {
  0x123, 0x456, 0x789, 0x987,
  0x654, 0x321, 0x111, 0x222,
  0x333, 0x444, 0x555, 0x666,
  0x777, 0x888, 0x999, 0xaaa };

// Important key, must _not_ be leaked
uint64_t top_secret = 0x5ec1237;

void computation_using_top_secret() {
  // Assume this function uses top_secret to authenticate with an external 
  // service
  return;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Missing first argument(integer). 13825 seems interesting.\n");
    return 0;
  }

  int offset = atoi(argv[1]);

  printf("The data you requested at offset %d is: 0x%lx\n", offset, dataset[offset]);
  computation_using_top_secret();

  return 0;
}
