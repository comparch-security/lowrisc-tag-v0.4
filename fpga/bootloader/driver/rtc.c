// See LICENSE for license details.

#include "rtc.h"

#define DEF_RTC_BASE_PTR(NUM) \
volatile uint64_t *rtc##NUM##_base_ptr = (uint64_t *)(RTC##NUM##_BASE);

#define DEF_RTC_CHECK_IRQQ(NUM) \
uint8_t rtc##NUM##_check_irq(void) { \
  uint64_t cmp = *(rtc##NUM##_base_ptr+1); \
  uint64_t time = *rtc##NUM##_base_ptr; \
  if(time > cmp) return 1; \
  return 0; \
}

#define DEF_RTC_WRITE_CMP(NUM) \
void rtc##NUM##_write_cmp(uint64_t cmptime) { \
  *(rtc##NUM##_base_ptr+1) = cmptime; \
}

#define DEF_RTC_UPDATE_CMP(NUM) \
void rtc##NUM##_update_cmp(uint64_t delta) { \
  *(rtc##NUM##_base_ptr+1) = (*rtc##NUM##_base_ptr) + delta; \
}

DEF_RTC_BASE_PTR(1)
DEF_RTC_BASE_PTR(2)
DEF_RTC_CHECK_IRQQ(1)
DEF_RTC_CHECK_IRQQ(2)
DEF_RTC_WRITE_CMP(1)
DEF_RTC_WRITE_CMP(2)
DEF_RTC_UPDATE_CMP(1)
DEF_RTC_UPDATE_CMP(2)
