// A hello world program

#include <stdio.h>
#include "uart.h"

#define write_csr(reg, val) \

#define STM_TRACE(id, value) \
  { \
    asm volatile ("mv   a0,%0": :"r" ((uint64_t)value) : "a0");	\
    asm volatile ("csrw 0x8ff, %0" :: "r"(id));	\
  }

static void trace_event0() {
  STM_TRACE(0, 0x42);
}

static void trace_event1(uint64_t value) {
  STM_TRACE(1, value);
}

static void trace_event2(uint64_t id, uint64_t value) {
  STM_TRACE(id, value);
}

int main() {
  long int i=0,j=0,k=0;
  STM_TRACE(0x1234, 0xdeadbeef);
  trace_event0();
  trace_event1(23);
  trace_event2(0xabcd, 0x0123456789abcdef);
  for(i=0;i<200;i++) {
    for(j=0;j<100000;j++);
      trace_event1(k++);
  }
  while(1) {
    for(j=0;j<1000000;j++);
      trace_event1(k++);
  }
}

