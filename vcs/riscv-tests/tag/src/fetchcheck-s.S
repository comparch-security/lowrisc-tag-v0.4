# See LICENSE for license details.

#*****************************************************************************
# fetchcheck.S
#-----------------------------------------------------------------------------
#
# Test exceptions when executing an instruction matching the fetchcheck mask
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
  li   x1, TMASK_STORE_PROP
  csrw utagctrl, x1

  # Apply tags to the appropriate point in each test
  la   x5, t2_0b0101
  li   x4, 0x5
  lw   x6, 0(x5)
  tagw x6, x4
  sw   x6, 0(x5)

  la   x5, t3_0b11
  li   x4, 0x3
  lw   x6, 0(x5)
  tagw x6, x4
  sw   x6, 0(x5)

  la   x5, t4_0b10
  li   x4, 0x2
  lw   x6, 0(x5)
  tagw x6, x4
  sw   x6, 0(x5)

  la   x5, t5_0b0100
  li   x4, 0x4
  lw   x6, 0(x5)
  tagw x6, x4
  sw   x6, 0(x5)

  la   x5, t6_0b01
  li   x4, 0x1
  lw   x6, 0(x5)
  tagw x6, x4
  sw   x6, 0(x5)

  li   x10, 0xf00

  # An exception should take place immediately if the mask matches, and the
  # tagged instruction should not be executed

  # make sure the update of tagctrl takes immediate effect as fetch check happens on ID stage
  li   TESTNUM, 2
  li   x1, 0x1
  slli x1, x1, TSHIM_FETCH_CHECK
.align 6
  csrw utagctrl, x1
t2_0b0101:
  li   x10, 0x0
  beq  x10, x0, fail


  li   TESTNUM, 3
  li   x1, 0x1
  slli x1, x1, TSHIM_FETCH_CHECK
  csrw utagctrl, x1
.align 3
t3_0b11:
  li   x10, 0x0
  beq  x10, x0, fail

  li   TESTNUM, 4
  li   x1, 0x2
  slli x1, x1, TSHIM_FETCH_CHECK
  csrw utagctrl, x1
.align 3
t4_0b10:
  li   x10, 0x0
  beq  x10, x0, fail

  # The 4-bit tag should be split across the two instruction in a 64-bit word
  li   TESTNUM, 5
  li   x1, 0x1
  slli x1, x1, TSHIM_FETCH_CHECK
  csrw utagctrl, x1
.align 3
t5_0b0100:
  nop
  li   x10, 0x0
  beq  x10, x0, fail

  # No exception should take place if the mask doesn't match
  li   TESTNUM, 6
  li   x1, 0x2
  slli x1, x1, TSHIM_FETCH_CHECK
  csrw utagctrl, x1
.align 3
t6_0b01:


  TEST_PASSFAIL

# Trap handler: if the excpetion was a tag exception that was expected for
# this test number, then jump over the next two instructions
stvec_handler:
  ENTER_TAG_SUPER
  csrr x17, scause
  li   x16, CAUSE_USER_ECALL
  bne  x16, x17, 1f
  j    pass
1:
  li   x16, 6
  beq  TESTNUM, x16, fail
  li   x16, CAUSE_TAG_CHECK_FAIL
  bne  x16, x17, fail
  csrr x16, sepc
  addi x16, x16, 4
  csrw sepc, x16
  EXIT_TAG_SUPER
  sret

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
