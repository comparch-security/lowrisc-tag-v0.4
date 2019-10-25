# See LICENSE for license details.

#*****************************************************************************
# storekeep.S
#-----------------------------------------------------------------------------
#
# Test support for 'sticky' in-memory tags
#

#define __TAG_MODE
#include "riscv_test.h"
#include "test_macros.h"

RVTEST_RV64U
RVTEST_CODE_BEGIN

  # Test setup
  li   x1, TMASK_LOAD_PROP
  csrw utagctrl, x1
  li   x1, TMASK_STORE_PROP
  csrs utagctrl, x1
  li   x1, 0x9
  slli x1, x1, TSHIM_STORE_KEEP
  csrs utagctrl, x1
  la   x10, tdat

  # Tags in the STORE_KEEP mask can be set as usual
  li   TESTNUM, 2
  li   x6, 0x9
  tagw x5, x6
  sd   x5, 0(x10)
  ld   x5, 0(x10)
  tagr x5, x5
  bne  x5, x6, fail

  # Any STORE_KEEP tags survive an attempt to overwrite the tag
  li   TESTNUM, 3
  tagw x5, x0
  sd   x5, 0(x10)
  ld   x5, 0(x10)
  tagr x5, x5
  bne  x5, x6, fail

  li   TESTNUM, 4
  tagw x5, x0
  sw   x5, 0(x10)
  ld   x5, 0(x10)
  tagr x5, x5
  bne  x5, x6, fail

  li   TESTNUM, 5
  tagw x5, x0
  sh   x5, 0(x10)
  ld   x5, 0(x10)
  tagr x5, x5
  bne  x5, x6, fail

  li   TESTNUM, 6
  tagw x5, x0
  sb   x5, 0(x10)
  ld   x5, 0(x10)
  tagr x5, x5
  bne  x5, x6, fail

  # STORE_KEEP should be unaffected by whether store propagation is enabled
  li   x1, TMASK_STORE_PROP
  csrc utagctrl, x1

  li   TESTNUM, 7
  tagw x5, x0
  sd   x5, 0(x10)
  ld   x5, 0(x10)
  tagr x5, x5
  bne  x5, x6, fail

  # test the slight chance that forwarding in D$ might affect the results
  li   TESTNUM, 8
  tagw x5, x0
.align 6
  sd   x5, 0(x10)
  ld   x5, 0(x10)
  tagr x5, x5
  bne  x5, x6, fail

  li   TESTNUM, 9
  tagw x5, x0
.align 6
  sd   x5, 0(x10)
  nop
  ld   x5, 0(x10)
  tagr x5, x5
  bne  x5, x6, fail

  li   TESTNUM, 10
  tagw x5, x0
.align 6
  sd   x5, 0(x10)
  nop
  nop
  ld   x5, 0(x10)
  tagr x5, x5
  bne  x5, x6, fail

  li   TESTNUM, 11
  tagw x5, x0
.align 6
  sd   x5, 0(x10)
  nop
  nop
  nop
  ld   x5, 0(x10)
  tagr x5, x5
  bne  x5, x6, fail

  TEST_PASSFAIL

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

tdat:   .dword 0x0000ffff0f0f0f0f

RVTEST_DATA_END
