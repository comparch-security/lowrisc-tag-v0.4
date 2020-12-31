// A hello world program

#include <stdio.h>
#include "uart.h"

int main() {
  long int i=0,j=0;
  uart_init();
  while(1) {
    for(i=0;i<100000;i++);
    printf("Hello World! %d\n",j++);
  }
}
