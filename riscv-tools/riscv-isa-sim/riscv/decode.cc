// See LICENSE for license details.

#include "decode.h"

freg_t::freg_t(const reg_t &r)
  : tagged_data_t(r.data, r.tag) {}

reg_t::reg_t(const freg_t &r)
  : tagged_data_t(r.data, r.tag) {}

reg_t::reg_t(const sreg_t &r)
  : tagged_data_t(r.data, r.tag) {}

