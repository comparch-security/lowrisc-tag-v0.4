# See LICENSE for license details.

#*****************************************************************************
# alu.S
#-----------------------------------------------------------------------------
#
# Test alu tag propagation.
#

#define __TAG_MODE
#include "riscv_test.h"
#include "test_macros.h"

RVTEST_RV64U
RVTEST_CODE_BEGIN

# disable tag control CSR
        csrw utagctrl, x0       ;

# x0 should have a zero tag
        TEST_CASE( 2, x5, 0,     \
        tagr x5, x0             ;\
        )

# x1 should have a zero initial tag
        TEST_CASE( 3, x5, 0,     \
        tagr x5, x1             ;\
        )

# enable alu propagation with 0x1
        li   x1, 0x1            ;
        slli x1, x1, TSHIM_ALU_PROP ;
        csrw utagctrl, x1       ;

# propagate zero tag
        TEST_CASE( 4, x5, 0,     \
        add  x1, x0, x1         ;\
        tagr x5, x1             ;\
        )

# propagate 0x1 tag
        TEST_CASE( 5, x5, 0x1,   \
        li   x1, 0x1            ;\
        tagw x1, x1             ;\
        add  x1, x0, x1         ;\
        tagr x5, x1             ;\
        )

# enable alu propagation with 0x2
        li   x2, 0x2            ;
        slli x2, x2, TSHIM_ALU_PROP ;
        csrw utagctrl, x2       ;

# propagate 0x2 tag
        TEST_CASE( 6, x5, 0x2,   \
        li   x2, 0x2            ;\
        tagw x2, x2             ;\
        add  x2, x1, x2         ;\
        tagr x5, x2             ;\
        )

# enable alu progagation with 0x3
        li   x3, 0x3            ;
        slli x3, x3, TSHIM_ALU_PROP ;
        csrw utagctrl, x3       ;

# propagate 0x3 tag
        TEST_CASE( 7, x5, 0x3,   \
        li   x3, 0x3            ;\
        and  x3, x1, x2         ;\
        tagr x5, x3             ;\
        )

# immediate instruction
        TEST_CASE( 8, x5, 0x3,   \
        andi x3, x3, 0x0        ;\
        tagr x5, x3             ;\
        )

# mov instruction
        TEST_CASE( 9, x5, 0x3,   \
        addi x4, x3, 0          ;\
        tagr x5, x4             ;\
        )

# alu's impact on tagr
        csrw utagctrl, x0       ;
        TEST_CASE(10, x5, 0x3,   \
        tagr x5, x4             ;\
        )

# alu's impact on tagw
        TEST_CASE(11, x5, 0xf,   \
        li   x2, 0xf            ;\
        tagw x4, x2             ;\
        tagr x5, x4             ;\
        )

# clear tag
        TEST_CASE(12, x5, 0x0,   \
        addi x3, x4, 0          ;\
        tagr x5, x3             ;\
        )

# exhaustive check of "alu"-class instructions. TINST_CHECK sets up an
# environment so x2 and x3 can be used as register inputs to the ALU
# operation, and x2 should be the desetination register

# enable alu propagation
        li   x1, 0xf            ;
        slli x1, x1, TSHIM_ALU_PROP ;
        csrw utagctrl, x1       ;

#define TINST_CHECK(tnum, expected, instr_line...) \
        TEST_CASE(tnum, x5, expected, \
        li x2, 0x3;\
        li x3, 0xc;\
        tagw x2, x2;\
        tagw x3, x3;\
        instr_line;\
        tagr x5, x4;\
        )

# reg-imm
        TINST_CHECK(13, 0x3, addi x4, x2, 11);
        TINST_CHECK(14, 0x3, addiw x4, x2, 11);
        TINST_CHECK(15, 0x3, andi x4, x2, 11);
        TINST_CHECK(16, 0x3, ori x4, x2, 11);
        TINST_CHECK(17, 0x3, slli x4, x2, 11);
        TINST_CHECK(18, 0x3, slliw x4, x2, 11);
        TINST_CHECK(19, 0x3, slti x4, x2, 11);
        TINST_CHECK(20, 0x3, sltiu x4, x2, 11);
        TINST_CHECK(21, 0x3, srai x4, x2, 11);
        TINST_CHECK(22, 0x3, sraiw x4, x2, 11);
        TINST_CHECK(23, 0x3, srli x4, x2, 11);
        TINST_CHECK(24, 0x3, srliw x4, x2, 11);

# reg-reg
        TINST_CHECK(25, 0xf, add x4, x2, x3);
        TINST_CHECK(26, 0xf, addw x4, x2, x3);
        TINST_CHECK(27, 0xf, and x4, x2, x3);
        TINST_CHECK(28, 0xf, div x4, x2, x3);
        TINST_CHECK(29, 0xf, divu x4, x2, x3);
        TINST_CHECK(30, 0xf, divuw x4, x2, x3);
        TINST_CHECK(31, 0xf, divw x4, x2, x3);
        TINST_CHECK(32, 0xf, mul x4, x2, x3);
        TINST_CHECK(33, 0xf, mulh x4, x2, x3);
        TINST_CHECK(34, 0xf, mulhsu x4, x2, x3);
        TINST_CHECK(35, 0xf, mulhu x4, x2, x3);
        TINST_CHECK(36, 0xf, mulw x4, x2, x3);
        TINST_CHECK(37, 0xf, or x4, x2, x3);
        TINST_CHECK(38, 0xf, rem x4, x2, x3);
        TINST_CHECK(39, 0xf, remu x4, x2, x3);
        TINST_CHECK(40, 0xf, remuw x4, x2, x3);
        TINST_CHECK(41, 0xf, remw x4, x2, x3);
        TINST_CHECK(42, 0xf, sll x4, x2, x3);
        TINST_CHECK(43, 0xf, sllw x4, x2, x3);
        TINST_CHECK(44, 0xf, slt x4, x2, x3);
        TINST_CHECK(45, 0xf, sltu x4, x2, x3);
        TINST_CHECK(46, 0xf, sra x4, x2, x3);
        TINST_CHECK(47, 0xf, sraw x4, x2, x3);
        TINST_CHECK(48, 0xf, srl x4, x2, x3);
        TINST_CHECK(49, 0xf, srlw x4, x2, x3);
        TINST_CHECK(50, 0xf, sub x4, x2, x3);
        TINST_CHECK(51, 0xf, subw x4, x2, x3);
        TINST_CHECK(52, 0xf, xor x4, x2, x3);

# imm
        TINST_CHECK(53, 0x0, lui x4, 513);
        TINST_CHECK(54, 0x0, auipc x4, 513);

  TEST_PASSFAIL

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END

