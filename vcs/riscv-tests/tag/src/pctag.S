# See LICENSE for license details.

#*****************************************************************************
# pctag.S
#-----------------------------------------------------------------------------
#
# Test pc tag.
#

#define __TAG_MODE
#include "riscv_test.h"
#include "test_macros.h"

RVTEST_RV64U
RVTEST_CODE_BEGIN

# normal branch
        li   TESTNUM, 2         ;
        li   x1, 1              ;
        bne  x1, x0, 1f         ;
        j    fail               ;
1:

# enable direct control flow target
        slli x3, x1, TSHIM_CFLOW_DIR_TGT ;
        li   x2, TMASK_STORE_PROP;
        or   x3, x3, x2         ;
        li   x2, TMASK_LOAD_PROP;
        or   x3, x3, x2         ;
        csrw utagctrl, x3       ;

# branch with pc tag check ok, cache miss
        li   TESTNUM, 3         ;
        la   x10, 1f            ;
        lw   x9, 0(x10)         ;
        tagw x9, x1             ;
        sw   x9, 0(x10)         ;
        fence.i                 ;
        bne  x1, x0, 1f         ;
        j    fail               ;
.align 6
1:

# branch with pc tag check ok, no cache miss
        li   TESTNUM, 4         ;
        la   x10, 1f            ;
        lw   x9, 0(x10)         ;
        tagw x9, x1             ;
        sw   x9, 0(x10)         ;
        fence.i                 ;
.align 6
        bne  x1, x0, 1f         ;
        j    fail               ;
.align 3
1:

# branch with pc tag check fail, cache miss
        li   TESTNUM, 5         ;
        bne  x1, x0, 1f         ;
        j    fail               ;
.align 6
1:      j    fail               ;

# branch with pc tag check fail, no cache miss
        li   TESTNUM, 6         ;
.align 6
        bne  x1, x0, 1f         ;
        j    fail               ;
.align 3
1:      j    fail               ;

# branch with pc tag check ok, force no take_pc
        li   TESTNUM, 7         ;
        la   x10, 1f            ;
        lw   x9, 0(x10)         ;
        tagw x9, x1             ;
        sw   x9, 0(x10)         ;
        li   x2, 16             ;
        fence.i                 ;
# train branch prediction and BTB if any
.align 3
1:      addi x2, x2, -1         ;
        bne  x2, x0, 1b         ;

# branch with not taken, force no take_pc
        li   TESTNUM, 8         ;
        la   x10, 1f            ;
        lw   x9, 0(x10)         ;
        tagw x9, x1             ;
        sw   x9, 0(x10)         ;
        li   x2, 16             ;
        fence.i                 ;
# train branch prediction and BTB if any
.align 3
2:      addi x2, x2, -1         ;
        beq  x2, x0, 1f         ;
        la   x3, 2b             ;
        jalr x0, x3, 0          ;
.align 3
1:

# jump with pc tag check ok
        li   TESTNUM, 9         ;
        la   x10, 1f            ;
        lw   x9, 0(x10)         ;
        tagw x9, x1             ;
        sw   x9, 0(x10)         ;
        fence.i                 ;
        j    1f                 ;
.align 3
1:


# jump with pc tag check fail
        li   TESTNUM, 10        ;
        j    1f                 ;
.align 3
1:      j    fail               ;

# jump with pc tag check ok, force no take_pc
        li   TESTNUM, 11        ;
        la   x10, 1f            ;
        lw   x9, 0(x10)         ;
        tagw x9, x1             ;
        sw   x9, 0(x10)         ;
        lw   x9, 16(x10)        ;
        tagw x9, x1             ;
        sw   x9, 16(x10)        ;
        li   x2, 16             ;
        fence.i                 ;
.align 3
1:      addi x2, x2, -1         ;
        beq  x2, x0, 1f         ;
        j    1b                 ;
        nop                     ;
1:

# jalr not set tag (not really needed)
        li   TESTNUM, 12        ;
        la   x3, 1b             ;
        jalr x0, x3, 16         ;

# disable direct control flow target
        slli x3, x1, TSHIM_CFLOW_DIR_TGT ;
        csrc utagctrl, x3       ;
        slli x3, x1, TSHIM_CFLOW_INDIR_TGT ;
        csrs utagctrl, x3       ;

# jalr with pc tag check ok
        li   TESTNUM, 13        ;
        la   x10, 1f            ;
        lw   x9, 0(x10)         ;
        tagw x9, x1             ;
        sw   x9, 0(x10)         ;
        la   x3, 1f             ;
        fence.i                 ;
        jalr x0, x3, 0          ;
.align 3
1:


# jalr with pc tag check fail
        li   TESTNUM, 14        ;
        la   x3, 1f             ;
        jalr x0, x3, 0          ;
.align 3
1:      j    fail               ;

# jalr with pc tag check ok, force no take_pc
        li   TESTNUM, 15        ;
        la   x10, 1f            ;
        lw   x9, 0(x10)         ;
        tagw x9, x1             ;
        sw   x9, 0(x10)         ;
        lw   x9, 16(x10)        ;
        tagw x9, x1             ;
        sw   x9, 16(x10)        ;
        li   x2, 16             ;
        fence.i                 ;
.align 3
1:      addi x2, x2, -1         ;
        beq  x2, x0, 1f         ;
        jalr x0, x10, 0         ;
        nop                     ;
1:

# make sure tag check fail will not override normal exceptions
        li   TESTNUM, 16        ;
        la   x10, 1f            ;
        lw   x15, 0(x10)        ;
        tagw x15, x1            ;
        sw   x0, 0(x10)         ;
        fence.i                 ;
        jalr x0, x10, 0         ;
.align 3
1:      nop                     ;
        j    fail               ;


        csrw utagctrl, x0       ;
        TEST_PASSFAIL

mtvec_handler:
        li   x16, 5             ;
        beq  TESTNUM, x16, 1f   ;
        li   x16, 6             ;
        beq  TESTNUM, x16, 1f   ;
        li   x16, 10            ;
        beq  TESTNUM, x16, 1f   ;
        li   x16, 14            ;
        beq  TESTNUM, x16, 1f   ;
        j    2f                 ;
# test case 5/6
1:      li   x16, CAUSE_TAG_CHECK_FAIL ;
        csrr x17, mcause        ;
        bne  x16, x17, fail     ;
        csrr x16, mepc          ;
        tagr x17, x16           ;
        bne  x17, x0, fail      ;
        addi x16, x16, 4        ;
        csrw mepc, x16          ;
        EXIT_TAG_MACHINE        ;
        mret                    ;
# test case 16
2:      li   x16, 16            ;
        beq  TESTNUM, x16, 1f   ;
        j    fail               ;
1:      csrr x17, mcause        ;
        li   x16, CAUSE_ILLEGAL_INSTRUCTION ;
        bne  x16, x17, 1f       ;
        csrr x17, mepc          ;
        lw   x16, 0(x17)        ;
        bne  x16, x0, fail      ;
        tagr x16, x16           ;
        bne  x16, x0, fail      ;
        mv   x16, x15           ;
        tagw x16, x0            ;
        sw   x16, 0(x17)        ;
        EXIT_TAG_MACHINE        ;
        fence.i                 ;
        mret                    ;
1:      li   x16, CAUSE_TAG_CHECK_FAIL ;
        bne  x16, x17, fail     ;
        csrr x17, mepc          ;
        mv   x16, x15           ;
        tagw x15, x0            ;
        sw   x15, 4(x17)        ;
        sw   x16, 0(x17)        ;
        EXIT_TAG_MACHINE        ;
        fence.i                 ;
        mret                    ;

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
