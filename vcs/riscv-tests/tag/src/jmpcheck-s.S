# See LICENSE for license details.

#*****************************************************************************
# jmpcheck.S
#-----------------------------------------------------------------------------
#
# Test exceptions when performing a jalr on an incorrectly tagged register
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

  # Test setup
test_start:
  li   x1, 0x9
  slli x1, x1, TSHIM_JMP_CHECK
  csrw utagctrl, x1

  # There should be no exception for jalr on a register with tags matching the
  # mask
  li   TESTNUM, 2
  li   x10, 0x1
  la   x5, t2tgt
  tagw x5, x10
  jalr x0, x5, 0
t2tgt:

  # Ensure that the jump doesn't take place if an exception is raised, and
  # that the exception is raised immediately
  li   TESTNUM, 3
  li   x6, 0xf00
  la   x5, t3tgt
  jalr x6, x5, 0
  li   x10, 0
  bne  x10, x0, t3suc
  li   x5,  0xf00
  beq  x6, x5, t3suc
t3tgt:
  j    fail
t3suc:


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
  li   x16, 3
  bne  TESTNUM, x16, fail
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

RVTEST_DATA_END
