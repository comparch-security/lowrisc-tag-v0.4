// See LICENSE for license details.

#ifndef FAN_HEADER_H
#define FAN_HEADER_H

#include <stdint.h>
#include "dev_map.h"

#ifdef DEV_MAP__io_ext_fan__BASE
  #define FAN_BASE ((uint32_t)(DEV_MAP__io_ext_fan__BASE))
  extern uint32_t temperature();
#else
  #define FAN_BASE 0
#endif

#endif
