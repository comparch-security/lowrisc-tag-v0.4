#include <stdio.h>
#include <stdint.h>
#include "encoding.h"

void set_tagged_val(uint64_t *dst, uint64_t val, uint8_t tag) {
  asm volatile ( "tagw %0, %1; sd %0, 0(%2); tagw %0, zero;" : : "r" (val), "r" (tag), "r" (dst));
}

uint8_t get_tagged_val(uint64_t *src, uint64_t *out) {
  uint8_t tag;
  uint64_t val;
  asm volatile( "ld %0, 0(%2); tagr %1, %0; tagw %0, zero;" : "=r" (val), "=r" (tag) : "r" (src));
  *out = val;
  return tag;
}

int main(void) {
  uint64_t utagctrl_val = read_csr(utagctrl);
  printf("utagctrl at entry: 0x%lx\n", utagctrl_val);
  utagctrl_val = TMASK_STORE_PROP | TMASK_LOAD_PROP | TMASK_LOAD_CHECK;
  write_csr(utagctrl, utagctrl_val);
  printf("Performing a write syscall\n");
  // ensure the syscall happens by flushing stdout, shouldn't be necessary if 
  // stdout it connected to a terminal
  fflush(stdout);

  // Set a tag on a memory location
  uint64_t stack_slot = -1;
  set_tagged_val(&stack_slot, 1337, 3);
  uint64_t read_val;
  uint8_t read_tag = get_tagged_val(&stack_slot, &read_val);
  printf("Read value 0x%lx from location 0x%p with tag 0x%x\n", read_val, &stack_slot, read_tag);

  utagctrl_val = read_csr(utagctrl);
  printf("utagctrl at exit: 0x%lx\n", utagctrl_val);
  return 0;
}
