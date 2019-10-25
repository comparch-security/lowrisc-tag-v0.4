# See LICENSE for license details.

#*****************************************************************************
# jmp.S
#-----------------------------------------------------------------------------
#
# Test setting of tags for jal/jalr
#

#define __TAG_MODE
#include "riscv_test.h"
#include "test_macros.h"

RVTEST_RV64U
RVTEST_CODE_BEGIN

  # Test setup
  li   x1, TMASK_JMP_PROP
  csrw utagctrl, x1
  li   x6, 0xf

  li   TESTNUM, 2
  addi x5, x0, 0
  jal  x5, t2tgt
t2tgt:
  tagr x5, x5
  bne  x5, x6, fail

  li   TESTNUM, 3
  addi x5, x0, 0
  la   x4, t3tgt
  jalr x5, x4, 0
t3tgt:
  tagr x5, x5
  bne  x5, x6, fail

  # Check the tag of the link register is set according to tagctrl
  csrw utagctrl, x0
  li   x6, 0x0

  li   TESTNUM, 4
  addi x5, x0, 0
  jal  x5, t4tgt
t4tgt:
  tagr x5, x5
  bne x5, x6, fail

  li   TESTNUM, 5
  addi x5, x0, 0
  la   x4, t5tgt
  jalr x5, x4, 0
t5tgt:
  tagr x5, x5
  bne x5, x6, fail

  # Check the tag of the link register is overwritten
  li   x6, 0xf

  li   TESTNUM, 6
  tagw x5, x6
  jal  x5, t6tgt
t6tgt:
  tagr x5, x5
  bne  x5, x0, fail

  li TESTNUM, 7
  tagw x5, x6
  la   x4, t7tgt
  jalr x5, x4, 0
t7tgt:
  tagr x5, x5
  bne  x5, x0, fail

  # enforce an I$ miss and do all the above tests again

  # Test setup
  li   x1, TMASK_JMP_PROP
  csrw utagctrl, x1
  li   x6, 0xf

  li   TESTNUM, 8
  addi x5, x0, 0
  jal  x5, t8tgt
.align 6
t8tgt:
  tagr x5, x5
  bne  x5, x6, fail

  li   TESTNUM, 9
  addi x5, x0, 0
  la   x4, t9tgt
  jalr x5, x4, 0
.align 6
t9tgt:
  tagr x5, x5
  bne  x5, x6, fail

  # Check the tag of the link register is set according to tagctrl
  csrw utagctrl, x0
  li   x6, 0x0

  li   TESTNUM, 10
  addi x5, x0, 0
  jal  x5, t10tgt
.align 6
t10tgt:
  tagr x5, x5
  bne x5, x6, fail

  li   TESTNUM, 11
  addi x5, x0, 0
  la   x4, t11tgt
  jalr x5, x4, 0
.align 6
t11tgt:
  tagr x5, x5
  bne x5, x6, fail

  # Check the tag of the link register is overwritten
  li   x6, 0xf

  li   TESTNUM, 12
  tagw x5, x6
  jal  x5, t12tgt
.align 6
t12tgt:
  tagr x5, x5
  bne  x5, x0, fail

  li TESTNUM, 13
  tagw x5, x6
  la   x4, t13tgt
  jalr x5, x4, 0
.align 6
t13tgt:
  tagr x5, x5
  bne  x5, x0, fail

  TEST_PASSFAIL

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
