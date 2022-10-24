// See LICENSE for license details.

#include "bio.h"

uint64_t get_sw(uint64_t data) {
  uint64_t *addr = DEV_MAP__io_int_sw__ADDR;
  return *addr;
}

uint64_t get_but(uint64_t data) {
  uint64_t *addr = DEV_MAP__io_int_but__ADDR;
  return *addr;
}

void set_led(uint64_t data) {
  uint64_t *addr = DEV_MAP__io_int_led__ADDR;
  *addr = data;
}
