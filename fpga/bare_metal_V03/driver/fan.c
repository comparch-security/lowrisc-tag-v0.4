
#include "fan.h"

volatile uint32_t *fan_base_ptr = (uint32_t *)(FAN_BASE);

uint32_t temperature() {
  return *(fan_base_ptr+1);
}
