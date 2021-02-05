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
  int i=0;
  unsigned long ctmen = CORETRACE_UPEN;
  ctmen |=  CORETRACE_JALR | CORETRACE_JAL;
  stm_trace(0,0);
  stm_trace(0,1);
  stm_trace(1,0);
  stm_trace(1,1);
  printf("hello world\n");
  write_csr(0x8fe,ctmen);  //enable trace
  drainout_ctmfifo();
  i=throw_test();
  write_csr(0x8fe,0);      //disable trace
  printf("throw_test return %d\n", i);
  return 1;
}
