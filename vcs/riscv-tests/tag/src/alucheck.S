# See LICENSE for license details.

#*****************************************************************************
# alucheck.S
#-----------------------------------------------------------------------------
#
# Test exceptions from tag ALU operations.
#

#define __TAG_MODE
#include "riscv_test.h"
#include "test_macros.h"

RVTEST_RV64U
RVTEST_CODE_BEGIN

  # Test setup
  li   x1, 0x9
  slli x1, x1, TSHIM_ALU_CHECK
  csrw utagctrl, x1

  # Exceptions should be raised even while propagation is disabled
  li   x2, 0x1
  li   x3, 0x1
  tagw x2, x2
  tagw x3, x3

  li   x10, 0xf

#define TINST_NOPROP(tnum, instr_line...) \
  li TESTNUM, tnum ;\
  instr_line       ;\
  li x10, 0        ;\
  beq x10, x0, fail;

  # reg-imm
  TINST_NOPROP(2, addi x4, x2, 11);
  TINST_NOPROP(3, addiw x4, x2, 11);
  TINST_NOPROP(4, andi x4, x2, 11);
  TINST_NOPROP(5, ori x4, x2, 11);
  TINST_NOPROP(6, slli x4, x2, 11);
  TINST_NOPROP(7, slliw x4, x2, 11);
  TINST_NOPROP(8, slti x4, x2, 11);
  TINST_NOPROP(9, sltiu x4, x2, 11);
  TINST_NOPROP(10, srai x4, x2, 11);
  TINST_NOPROP(11, sraiw x4, x2, 11);
  TINST_NOPROP(12, srli x4, x2, 11);
  TINST_NOPROP(13, srliw x4, x2, 11);

  # reg-reg
  TINST_NOPROP(14, add x4, x2, x3);
  TINST_NOPROP(15, addw x4, x2, x3);
  TINST_NOPROP(16, and x4, x2, x3);
  TINST_NOPROP(17, div x4, x2, x3);
  TINST_NOPROP(18, divu x4, x2, x3);
  TINST_NOPROP(19, divuw x4, x2, x3);
  TINST_NOPROP(20, divw x4, x2, x3);
  TINST_NOPROP(21, mul x4, x2, x3);
  TINST_NOPROP(22, mulh x4, x2, x3);
  TINST_NOPROP(23, mulhsu x4, x2, x3);
  TINST_NOPROP(24, mulhu x4, x2, x3);
  TINST_NOPROP(25, mulw x4, x2, x3);
  TINST_NOPROP(26, or x4, x2, x3);
  TINST_NOPROP(27, rem x4, x2, x3);
  TINST_NOPROP(28, remu x4, x2, x3);
  TINST_NOPROP(29, remuw x4, x2, x3);
  TINST_NOPROP(30, remw x4, x2, x3);
  TINST_NOPROP(31, sll x4, x2, x3);
  TINST_NOPROP(32, sllw x4, x2, x3);
  TINST_NOPROP(33, slt x4, x2, x3);
  TINST_NOPROP(34, sltu x4, x2, x3);
  TINST_NOPROP(35, sra x4, x2, x3);
  TINST_NOPROP(36, sraw x4, x2, x3);
  TINST_NOPROP(37, srl x4, x2, x3);
  TINST_NOPROP(38, srlw x4, x2, x3);
  TINST_NOPROP(39, sub x4, x2, x3);
  TINST_NOPROP(40, subw x4, x2, x3);
  TINST_NOPROP(41, xor x4, x2, x3);

  # make sure tag check is on rs1_tag | rs2_tag
  TINST_NOPROP(114, add x4, x2, x0);
  TINST_NOPROP(115, addw x4, x2, x0);
  TINST_NOPROP(116, and x4, x2, x0);
  TINST_NOPROP(117, div x4, x2, x0);
  TINST_NOPROP(118, divu x4, x2, x0);
  TINST_NOPROP(119, divuw x4, x2, x0);
  TINST_NOPROP(120, divw x4, x2, x0);
  TINST_NOPROP(121, mul x4, x2, x0);
  TINST_NOPROP(122, mulh x4, x2, x0);
  TINST_NOPROP(123, mulhsu x4, x2, x0);
  TINST_NOPROP(124, mulhu x4, x2, x0);
  TINST_NOPROP(125, mulw x4, x2, x0);
  TINST_NOPROP(126, or x4, x2, x0);
  TINST_NOPROP(127, rem x4, x2, x0);
  TINST_NOPROP(128, remu x4, x2, x0);
  TINST_NOPROP(129, remuw x4, x2, x0);
  TINST_NOPROP(130, remw x4, x2, x0);
  TINST_NOPROP(131, sll x4, x2, x0);
  TINST_NOPROP(132, sllw x4, x2, x0);
  TINST_NOPROP(133, slt x4, x2, x0);
  TINST_NOPROP(134, sltu x4, x2, x0);
  TINST_NOPROP(135, sra x4, x2, x0);
  TINST_NOPROP(136, sraw x4, x2, x0);
  TINST_NOPROP(137, srl x4, x2, x0);
  TINST_NOPROP(138, srlw x4, x2, x0);
  TINST_NOPROP(139, sub x4, x2, x0);
  TINST_NOPROP(140, subw x4, x2, x0);
  TINST_NOPROP(141, xor x4, x2, x0);

# Enable ALU propagation. tagr/tagw should still not trigger an exception
  li   x1, TMASK_ALU_PROP
  csrs utagctrl, x1
  li   TESTNUM, 42
  li   x2, 0x8
  li   x3, 0x8
  tagw x2, x2
  tagw x3, x3

  # An exception should be raised, and the destination register and its tag
  # should remain unmodified

#define TINST_PROP(tnum, instr_line...) \
  li TESTNUM, tnum   ;\
  li x4, 0xf00d      ;\
  instr_line         ;\
  li x10, 0          ;\
  beq x10, x0, fail  ;\
  li x5, 0xf00d     ;\
  bne x5, x4, fail  ;\
  tagr x5, x4       ;\
  bne x5, zero, fail;

  # reg-imm
  TINST_PROP(43, addi x4, x2, 11);
  TINST_PROP(44, addiw x4, x2, 11);
  TINST_PROP(45, andi x4, x2, 11);
  TINST_PROP(46, ori x4, x2, 11);
  TINST_PROP(47, slli x4, x2, 11);
  TINST_PROP(48, slliw x4, x2, 11);
  TINST_PROP(49, slti x4, x2, 11);
  TINST_PROP(50, sltiu x4, x2, 11);
  TINST_PROP(51, srai x4, x2, 11);
  TINST_PROP(52, sraiw x4, x2, 11);
  TINST_PROP(53, srli x4, x2, 11);
  TINST_PROP(54, srliw x4, x2, 11);

  # reg-reg
  TINST_PROP(55, add x4, x2, x3);
  TINST_PROP(56, addw x4, x2, x3);
  TINST_PROP(57, and x4, x2, x3);
  TINST_PROP(58, div x4, x2, x3);
  TINST_PROP(59, divu x4, x2, x3);
  TINST_PROP(60, divuw x4, x2, x3);
  TINST_PROP(61, divw x4, x2, x3);
  TINST_PROP(62, mul x4, x2, x3);
  TINST_PROP(63, mulh x4, x2, x3);
  TINST_PROP(64, mulhsu x4, x2, x3);
  TINST_PROP(65, mulhu x4, x2, x3);
  TINST_PROP(66, mulw x4, x2, x3);
  TINST_PROP(67, or x4, x2, x3);
  TINST_PROP(68, rem x4, x2, x3);
  TINST_PROP(69, remu x4, x2, x3);
  TINST_PROP(70, remuw x4, x2, x3);
  TINST_PROP(71, remw x4, x2, x3);
  TINST_PROP(72, sll x4, x2, x3);
  TINST_PROP(73, sllw x4, x2, x3);
  TINST_PROP(74, slt x4, x2, x3);
  TINST_PROP(75, sltu x4, x2, x3);
  TINST_PROP(76, sra x4, x2, x3);
  TINST_PROP(77, sraw x4, x2, x3);
  TINST_PROP(78, srl x4, x2, x3);
  TINST_PROP(79, srlw x4, x2, x3);
  TINST_PROP(80, sub x4, x2, x3);
  TINST_PROP(81, subw x4, x2, x3);
  TINST_PROP(82, xor x4, x2, x3);


  TEST_PASSFAIL

# Trap handler: if the exception was a tag exception that was expected for
# this test number, then jump over the next instruction
mtvec_handler:
  li   x16, 42
  beq  TESTNUM, x16, fail
  # All other tests expect a tag exception
  li   x16, CAUSE_TAG_CHECK_FAIL
  csrr x17, mcause
  bne  x16, x17, fail
  csrr x16, mepc
  addi x16, x16, 8
  csrw mepc, x16
  EXIT_TAG_MACHINE
  mret

RVTEST_CODE_END

.align 6
.data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
