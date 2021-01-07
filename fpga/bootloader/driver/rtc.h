// See LICENSE for license details.

#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include "dev_map.h"

// Xilinx AXI_QUAD_SPI

#ifdef DEV_MAP__io_int_rtc__BASE
  #define RTC1_BASE ((uint64_t)DEV_MAP__io_int_rtc__BASE)
#else
  #define RTC1_BASE 0
#endif

#ifdef DEV_MAP__io_int_rtc2__BASE
  #define RTC2_BASE ((uint64_t)DEV_MAP__io_int_rtc2__BASE)
#else
  #define RTC2_BASE 0
#endif


uint8_t rtc_check_irq(void);
void rtc_write_cmp(uint64_t cmptime);
void rtc_update_cmp(uint64_t delta);




#endif