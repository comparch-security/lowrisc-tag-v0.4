// A hello world program

#include <stdio.h>
#include "uart.h"
#include "fan.h"

int main() {
  long int i=0,j=0;
  uart_init();
  while(1) {
    for(i=0;i<100000;i++);
#ifdef DEV_MAP__io_ext_fan__BASE
    printf("Board temperature=%d\n", temperature());
#else
    printf("Hello World! %d\n", j++);
#endif
  }
}
