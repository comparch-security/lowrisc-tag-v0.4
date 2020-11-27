// See LICENSE for license details.

#include "rtc.h"

volatile uint64_t *rtc1_base_ptr = (uint64_t *)(RTC1_BASE);
volatile uint64_t *rtc2_base_ptr = (uint64_t *)(RTC2_BASE);

uint8_t rtc_check_irq(void) {
  uint64_t cmp = *(rtc2_base_ptr+1);
  uint64_t time = *rtc2_base_ptr;
  if(time > cmp)  return 1;
  return 0;
}

void rtc_update_cmp(uint64_t delta) {
  *(rtc2_base_ptr+1) = (*rtc2_base_ptr) + delta;
}

/*
uint8_t check_rtc_irq(void) {
  uint64_t cmp = *(rtc1_base_ptr+1);
  uint64_t time = *rtc1_base_ptr;
  if(time > cmp)  return 1;
  return 0;
}

void update_rtc_cmp(uint64_t delta) {
  *(rtc2_base_ptr+1) = (*rtc1_base_ptr) + delta;
}
*/
