// See LICENSE for license details.


#include "util.h"
#include <stdlib.h>
#include <stdio.h>

#define NUM_TAGS 1024
#define ADDR_MASK 0x03FFFFF8
#define TAG_MASK 0xF

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

static uint8_t * tags[NUM_TAGS];
static uint8_t * buf = (uint8_t *)(0x41000000llu);
static uint64_t ran_num = 0x729ac5b1519fac05;

uint64_t random() {
  // 64: 64, 63, 61, 60
  uint64_t m = ran_num & 0x1;
  ran_num = (ran_num >> 1) ^ (m << 63) ^ (m << 62) ^ (m << 60) ^ (m << 59);
  return ran_num;
}

uint8_t * store_test(uint64_t addr) {
  uint8_t *p = buf+addr;
  if(*p == 1) return NULL;
  *p = 1;
  store_tag(p, addr>>3);
  return p;
}

uint64_t load_test(uint64_t addr) {
  uint8_t *p = buf+addr;
  if(load_tag(p) != ((addr>>3) & TAG_MASK)) return 1;
  *p = 0;
  store_tag(p, 0);
  return 0;
}

int main( int argc, char* argv[] )
{

  uint64_t tag = 0;
  uint8_t *p;
  uint64_t addr;
  uint64_t i;
  uint64_t rp=0, wp=0;
  uint64_t cnt =0;
  uint8_t buf_valid = 0;

  // enable tag ALU/LOAD/STORE Propagartion
  uint64_t tag_mask = TMASK_ALU_PROP|TMASK_LOAD_PROP|TMASK_STORE_PROP;
  write_csr(utagctrl, tag_mask);

  // this is improper

  while(cnt++ < 10000) {
    p = tags[rp];
    addr = (uint64_t)(p) - (uint64_t)(buf);

    // check whether read 50%
    if(buf_valid && (random() & 0x1)) {
      if(load_test(addr)) return addr;
      rp = (rp + 1) % NUM_TAGS;
      if(rp == wp) buf_valid = 0;
    }

    // check whether write 75%
    if(!buf_valid || rp != wp || (random() & 0x11)) {
      addr = random() & ADDR_MASK;
      p = store_test(addr);
      if(p != NULL) {
        tags[wp] = p;
        wp = (wp + 1) % NUM_TAGS;
        buf_valid = 1;
      }      
    }
  }
}

