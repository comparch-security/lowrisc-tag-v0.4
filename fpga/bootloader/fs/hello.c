#include <stdio.h>
#include <stdint.h>
#include "encoding.h"
#define drainout_ctmfifo() asm volatile (".rept 10000;" "nop;" ".endr;")

int throw_test(void) {
  drainout_ctmfifo(); 
  try {
    drainout_ctmfifo(); 
    throw 1;
  }
  catch (...) {
    drainout_ctmfifo(); 
    return 1;
  }
  return 10;
}

int main(void) {
  stm_trace(0,0);
  stm_trace(0,1);
  stm_trace(1,0);
  stm_trace(1,1);
  drainout_ctmfifo();       
  printf("hello world\n");
  int i=0;
  i=throw_test();
  drainout_ctmfifo(); 
  printf("throw_test return %d\n", i);
  drainout_ctmfifo(); 
  return 1;
}
