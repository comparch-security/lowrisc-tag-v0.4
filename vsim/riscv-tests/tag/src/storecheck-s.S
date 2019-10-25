# See LICENSE for license details.

#*****************************************************************************
# storecheck.S
#-----------------------------------------------------------------------------
#
# Test exceptions when storing to locations with certain tag values.
#

#define __TAG_MODE
#include "riscv_test.h"
#include "test_macros.h"

RVTEST_RV64S
RVTEST_CODE_BEGIN

#ifdef __MACHINE_MODE
  #define sscratch mscratch
  #define sstatus mstatus
  #define scause mcause
  #define sepc mepc
  #define sret mret
  #define stvec_handler mtvec_handler
#endif

  # enable delegation and change into user mode
  li t0, SSTATUS_SPP
  csrc sstatus, t0
  la t0, test_start
  csrw sepc, t0
  sret


test_start:

  # Test setup
  la   x10, tdat
  li   x1, TMASK_LOAD_PROP
  csrw utagctrl, x1
  li   x1, TMASK_STORE_PROP
  csrs utagctrl, x1
  li   x1, 0x9
  slli x1, x1, TSHIM_STORE_CHECK
  csrs utagctrl, x1

  # Stores should be unaffected when the destination address has no tags
  li   TESTNUM, 2
  sd   x0, 0(x10)

  # The store check should be performed based on the tag of the target memory
  # location rather than the tag of the data being written
  li   TESTNUM, 3
  li   x5, 0xf
  tagw x5, x5
  sd   x5, 64(x10)

  # make sure fence.i is safe to stall pipeline until tag check is resolved
  li   TESTNUM, 4
  sd   x0, 64(x10)
  fence.i
  li   x10, 0
  beq  x10, x0, fail

  # set up tag check for different store width
  sd   x5, 128(x10)
  fence.i

  # Attempts to store to tdat should now trigger an exception
  li   TESTNUM, 5
  sd   x0, 128(x10)
  li   x10, 0
  nop
  beq  x10, x0,  fail

  li   TESTNUM, 6
  sw   x0, 128(x10)
  li   x10, 0
  nop
  beq  x10, x0,  fail

  li   TESTNUM, 7
  sh   x0, 128(x10)
  li   x10, 0
  nop
  beq  x10, x0,  fail

  li   TESTNUM, 8
  sb   x0, 128(x10)
  li   x10, 0
  nop
  beq  x10, x0,  fail

  # The failed attempts to write to the tagged location should not have changed
  # the memory contents
  li   TESTNUM, 9
  li   x29, 0xf
  ld   x5, 64(x10)
  bne  x5, x29, fail
  ld   x5, 128(x10)
  bne  x5, x29, fail

  # Storing to a location with a tag that doesn't match the 'storecheck' mask
  # shouldn't cause an exception
  li   TESTNUM, 10
  la   x10, tdat2
  li   x5, 0x6
  tagw x5, x5
  sd   x5, 0(x10)
  li   x5, 0xf
  tagw x5, x5
  sd   x5, 0(x10)


  TEST_PASSFAIL

# Trap handler: if the excpetion was a tag exception that was expected for
# this test number, then jump over the next instruction
stvec_handler:
  ENTER_TAG_SUPER
  csrr x17, scause
  li   x16, CAUSE_USER_ECALL
  bne  x16, x17, 1f
  j    pass
1:
  li   x16, 4
  beq  TESTNUM, x16, expected_tag_xcpt
  li   x16, 5
  beq  TESTNUM, x16, expected_tag_xcpt
  li   x16, 6
  beq  TESTNUM, x16, expected_tag_xcpt
  li   x16, 7
  beq  TESTNUM, x16, expected_tag_xcpt
  li   x16, 8
  beq  TESTNUM, x16, expected_tag_xcpt
  j    fail

expected_tag_xcpt:
  li   x16, CAUSE_TAG_CHECK_FAIL
  bne  x16, x17, fail
  csrr x16, sepc
  addi x16, x16, 12
  csrw sepc, x16
  EXIT_TAG_SUPER
  sret

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

  tdat:  .dword 0x0000ffff0f0f0f0f
.align 6
         .dword 0x0000000000000000
.align 6
         .dword 0x0000000000000000
  tdat2: .dword 0x0000ffff0f0f0f0f

RVTEST_DATA_END
