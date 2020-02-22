#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "encoding.h"

#define set_tagged_val(dst, val, tag) \
  asm volatile ( "tagw %0, %1; sd %0, 0(%2); tagw %0, zero;" : : "r" (val), "r" (tag), "r" (dst))
#define disable_tag_rules() write_csr(utagctrl, 0)
#define enable_tag_rules() write_csr(utagctrl, TMASK_STORE_PROP | TMASK_LOAD_CHECK)


uint64_t dataset[] = {
  0x123, 0x456, 0x789, 0x987,
  0x654, 0x321, 0x111, 0x222,
  0x333, 0x444, 0x555, 0x666,
  0x777, 0x888, 0x999, 0xaaa };

// Important key, must _not_ be leaked
uint64_t top_secret = 0x5ec1237;

// Protect secret value with a tag
static void init_tags() {
  fprintf(stderr, "setting tagged val at %p\n", &top_secret);
  set_tagged_val(&top_secret, top_secret, 0x1);
}

void computation_using_top_secret() {
  // Assume this function uses top_secret to authenticate with an external 
  // service
  return;
}

int main(int argc, char **argv) {
  /* Reading a tagged location triggers an exception */
  enable_tag_rules();

  init_tags(); // Would usually be performed by loader
  if (argc < 2) {
    printf("Missing first argument(integer). 13832 seems interesting.\n");
    return 0;
  }

  int offset = atoi(argv[1]);

  printf("Looking at location %p\n", dataset+offset);
  printf("The data you requested at offset %d is: 0x%lx\n", offset, dataset[offset]);
  computation_using_top_secret();

  return 0;
}
