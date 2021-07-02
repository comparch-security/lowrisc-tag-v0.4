// A dram test program

#include <stdio.h>
#include <stdlib.h>
#include "uart.h"
#include "memory.h"

static uint64_t lfsr(uint64_t x)
{
  uint64_t bit = (x ^ (x >> 1)) & 1;
  return (x >> 1) | (bit << 62);
}

#define TAG_MASK 0x3

int load_tag(void *addr) {
  int rv;
  asm volatile ("lw %0, 0(%1); tagr %0, %0"
                :"=r"(rv)
                :"r"(addr)
                );
  return rv;
}


void store_tag(void *addr, int tag) {
  asm volatile ("tagw %0, %0; andi %0, %0, 0; amoor.w %0, %0, 0(%1)"
                :
                :"r"(tag), "r"(addr)
                );
}

#define RANG 0x10000
#define SFN (RANG >> 0)
#define SHF 3

int main() {

  uart_init();
  printf("DRAM test program.\n");
  uint64_t *base = get_ddr_base();

  int i = 0;
  int s = 1;
  uint64_t seed = 0x2021;

  // initial
  for(i=0; i<RANG; i++) {
    *(base + i) = i;
    *(base + RANG + (i<<SHF)) = seed;
  }

  // test
  while(1) {
    printf("test use seed: %llx\n", seed);
    
    // shuffle
    for(i=0; i<SFN; i++) {
      s = lfsr(s);
      uint64_t offset = s % RANG;
      uint64_t m = *(base + offset);
      *(base + offset) = *(base + i);
      *(base + i) = m;
      if(i != offset && *(base + i) == *(base + offset)) {
        printf("Shuffle: %llx@%p <> %llx@%p with seed %llx\n", *(base + i), base + i, *(base + offset), base + offset, seed);
        exit(3);
      }
    }

    // check
    for(i=0; i<RANG; i++) {
      if(seed != *(base + RANG + (i<<SHF))) {
        printf("Error! %llx stored @0x%p does not match with %llx\n", *(base + RANG + (i<<SHF)), base + RANG + (i<<SHF), seed);
        exit(1);
      }
    }

    // refill
    seed = lfsr(seed);
    for(i=0; i<RANG; i++) {
      uint64_t *p = base + RANG + (*(base + i) << SHF);
      *p = seed;
    }
  }

}

