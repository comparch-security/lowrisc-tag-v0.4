// See LICENSE for license details.

#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include "dev_map.h"

// Xilinx AXI_QUAD_SPI
//rtc1_irq is timer_interrupt mtip
//rtc2_irq not same as rtc1_irq share port with uart_irq/spi_irq reference LowRISCChip.scala:285

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

#define RTC_CHECK_IRQQ(NUM) \
uint8_t rtc##NUM##_check_irq(void);

#define RTC_WRITE_CMP(NUM) \
void rtc##NUM##_write_cmp(uint64_t cmptime);

#define RTC_UPDATE_CMP(NUM) \
void rtc##NUM##_update_cmp(uint64_t delta);

RTC_CHECK_IRQQ(1)
RTC_CHECK_IRQQ(2)
RTC_WRITE_CMP(1)
RTC_WRITE_CMP(2)
RTC_UPDATE_CMP(1)
RTC_UPDATE_CMP(2)


#endif