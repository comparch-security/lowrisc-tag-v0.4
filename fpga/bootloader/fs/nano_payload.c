#include <stdint.h>
#include "encoding.h"
//osd-cli stm log stm.log 5

int main(void)
{
  stm_trace(0,0);
  stm_trace(0,1);
  stm_trace(1,0);
  stm_trace(1,1);
  return 1;
}
