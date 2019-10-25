# See LICENSE for license details.

#*****************************************************************************
# loadcheck.S
#-----------------------------------------------------------------------------
#
# Test tagged related load tag check.
#

#define __TAG_MODE
#include "riscv_test.h"
#include "test_macros.h"

RVTEST_RV64U
RVTEST_CODE_BEGIN

  # Test setup
  la   x10, tdat
  li   x1, TMASK_LOAD_PROP
  csrw utagctrl, x1
  li   x1, TMASK_STORE_PROP
  csrs utagctrl, x1
  li   x1, 0x9
  slli x1, x1, TSHIM_LOAD_CHECK
  csrs utagctrl, x1

  # Loads should be unaffected when the destination address has no tags
  li   TESTNUM, 2
  ld   x0, 0(x10)

  # The load check should be performed based on the tag of the target memory
  # location rather than the tag of register used for the address
  li   TESTNUM, 3
  addi x5, x10, 0
  li   x6, 0xf
  tagw x5, x6
  ld   x5, 0(x5)

  # Loading from a location with a tag that doesn't match the 'loadcheck' mask
  # shouldn't cause an exception
  li   TESTNUM, 4
  la   x10, tdat2
  li   x5, 0x6
  tagw x5, x5
  sd   x5, 0(x10)
  ld   x5, 0(x10)

  # All tests from this point on (other than test 10) are expected to trigger a tag exception
  # NOTE: fences will become unnecessary once the hardware generates precise
  # tag exceptions

  # enforce a cache miss to ensure fence.i is strong enough
  li   TESTNUM, 5
  li   x5, 0x0f
  li   x6, 0x9
  tagw x5, x6
  li   x6, 0xd00d
.align 6
  sd   x5, 64(x10)
  tagw x5, x0
  mv   x5, x6
  ld   x5, 64(x10)
  fence.i
  li   x5, 0x0f
  bne  x5, x6, fail

  # Attempts to load from a tagged tdat should trigger an immediate exception
  li   TESTNUM, 6
  ld   x5, 64(x10)
  li   x5, 0x0f
  nop
  bne  x5, x6, fail

  li   TESTNUM, 7
  lw   x5, 64(x10)
  li   x5, 0x0f
  nop
  bne  x5, x6, fail

  li   TESTNUM, 8
  lh   x5, 64(x10)
  li   x5, 0x0f
  nop
  bne  x5, x6, fail

  li   TESTNUM, 9
  lb   x5, 64(x10)
  li   x5, 0x0f
  nop
  bne  x5, x6, fail

  # Loads that failed the tag check should leave x5 unmodified
  li   TESTNUM, 10
  bne  x5, x6, fail

  # Exceptions should be triggered regardless of whether load propagation is
  # enabled
  li   x1, TMASK_LOAD_PROP
  csrc utagctrl, x1

  li   TESTNUM, 11
  lw   x0, 64(x10)
  li   x5, 0x0f
  nop
  bne  x5, x6, fail

  li   TESTNUM, 12
  lw   x0, 64(x10)
  li   x5, 0x0f
  nop
  bne  x5, x6, fail

  li   TESTNUM, 13
  lh   x0, 64(x10)
  li   x5, 0x0f
  nop
  bne  x5, x6, fail

  li   TESTNUM, 14
  lb   x0, 64(x10)
  li   x5, 0x0f
  nop
  bne  x5, x6, fail


  TEST_PASSFAIL

# Trap handler: if the exception was a tag exception that was expected for
# this test number, then jump over the next instruction
mtvec_handler:
  li   x16, 10
  beq  TESTNUM, x16, fail
  li   x16, 4
  bge  TESTNUM, x16, expected_tag_xcpt
  j    fail

expected_tag_xcpt:
  li   x16, CAUSE_TAG_CHECK_FAIL
  csrr x17, mcause
  bne  x16, x17, fail
  csrr x16, mepc
  addi x16, x16, 12
  csrw mepc, x16
  EXIT_TAG_MACHINE
  mret

RVTEST_CODE_END

.align 6
.data
RVTEST_DATA_BEGIN

  TEST_DATA

tdat:   .dword 0x0000ffff0f0f0f0f
.align 6
        .dword 0x0000000000000000
tdat2:  .dword 0x000ffff0f0f0f0f1


RVTEST_DATA_END
