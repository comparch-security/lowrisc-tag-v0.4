// See LICENSE for license details.

#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include "dev_map.h"

#define DEV_MAP__io_int_sw__OFFSET  0x00
#define DEV_MAP__io_int_but__OFFSET 0x08
#define DEV_MAP__io_int_led__OFFSET 0x10

#define DEV_MAP__io_int_sw__ADDR  (DEV_MAP__io_int_bio__BASE + DEV_MAP__io_int_sw__OFFSET )
#define DEV_MAP__io_int_but__ADDR (DEV_MAP__io_int_bio__BASE + DEV_MAP__io_int_but__OFFSET)
#define DEV_MAP__io_int_led__ADDR (DEV_MAP__io_int_bio__BASE + DEV_MAP__io_int_led__OFFSET)

void set_led(uint64_t data);

#endif
