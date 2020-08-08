# See LICENSE for license details.

#*****************************************************************************
# simple.S
#-----------------------------------------------------------------------------
#
# This is the most basic self checking test. If your simulator does not
# pass thiss then there is little chance that it will pass any of the
# more complicated self checking tests.
#

#include "riscv_test.h"
#include "test_macros.h"

#define L1IBLK    64
#define L1IBLKW   6
#define L1ISET    32
#define L1ISETW   5
#define L1DBLK    64
#define L1DBLKW   6
#define L1DSET    32
#define L1DSETW   5
#define L2BLK     64
#define L2BLKW    6
#define L2SET     128
#define L2SETW    7
#define L2BANKW   0


#define PFC_L1IR_TEST           \
    .rept 10000                 \
      nop;                      \
    .endr;                      

#define PFC_L1IRM_TEST          \
    auipc x5, 0;                \
    addi  x5, x5, 8;            \
    .rept 1000                  \
      addi  x5, x5, L1IBLK;     \
      jalr  x0, x5;             \
      .set nops, L1IBLK/4-2;    \
      .rept nops                \
        nop;                    \
      .endr;                    \
    .endr;                      \
    nop;


#define PFC_L1IRM2_TEST              \
    .set nops,  L1IBLK*L1ISET/4-1;   \
    addi  x5, x0, 100;               \
    L0: j L1; .rept nops nop; .endr; \
    L1: j L2; .rept nops nop; .endr; \
    L2: j L3; .rept nops nop; .endr; \
    L3: j L4; .rept nops nop; .endr; \
    L4: j L5; .rept nops nop; .endr; \
    L5: j L6; .rept nops nop; .endr; \
    L6: j L7; .rept nops nop; .endr; \
    L7: j L8; .rept nops nop; .endr; \
    L8: j L9; .rept nops nop; .endr; \
    L9: j La; .rept nops nop; .endr; \
    La: j Lb; .rept nops nop; .endr; \
    Lb: j Lc; .rept nops nop; .endr; \
    Lc: j Ld; .rept nops nop; .endr; \
    Ld: j Le; .rept nops nop; .endr; \
    Le: j Lf; .rept nops nop; .endr; \
    Lf: addi x5, x5, -1;             \
        beq  x5, x0, Finish;         \
        j  L0;                       \
    Finish: nop

#define PFC_L1DR_TEST           \
    addi x5, x0, PFC_MEM;       \
    slli x5, x5, 28;            \
    .rept 2                     \
      addi x5, x5, -1000;       \
      .set offset, 1000;        \
      .set offstep, 1;          \
      .rept 1000;               \
        offset=offset-offstep;  \
        lb x6, offset(x5);      \
      .endr;                    \
     .endr;
                      

#define PFC_L1DW_TEST           \
    addi x5, x0, PFC_MEM;       \
    slli x5, x5, 28;            \
    .rept 2                     \
      addi x5, x5, -1000;       \
      .set offset, 1000;        \
      .set offstep, 1;          \
      .rept 1000                \
        offset=offset-offstep;  \
        sb x6, offset(x5);      \
      .endr;                    \
     .endr;                     

#define PFC_L1DWB_TEST               \
    .set offsetw, L1DBLKW+L1DSETW;   \
    addi x5, x0, PFC_MEM;            \
    slli x5, x5, 28;            \
    addi x7, x0, -1;            \
    slli x7, x7, offsetw;       \
    .rept 1                     \
      .rept 1000                \
        add  x5, x5, x7;        \
        sw   x6, 0(x5);         \
      .endr;                    \
     .endr;                     

#define PFC_L2R_TEST            \
    addi x5, x0, PFC_MEM;       \
    slli x5, x5, 28;            \
    .rept 100                   \
      addi x5, x5, -640;        \
      .set offset, 640;         \
      .set offstep, 64;         \
      .rept 10                  \
        offset=offset-offstep;  \
        lw x6, offset(x5);      \
      .endr;                    \
     .endr;  


#define PFC_L2RB_TEST                \
    .set offsetw, L2BLKW+L2SETW;     \
    addi x5, x0, PFC_MEM;            \
    slli x5, x5, 28;            \
    addi x7, x0, -1;            \
    slli x7, x7, offsetw;       \
    .rept 1                     \
      .rept 1000                \
        add  x5, x5, x7;        \
        lw   x6, 0(x5);         \
      .endr;                    \
     .endr; 

#define PFC_L2WB_TEST                \
    .set offsetw, L2BLKW+L2SETW;     \
    addi x5, x0, PFC_MEM;            \
    slli x5, x5, 28;            \
    addi x7, x0, -1;            \
    slli x7, x7, offsetw;       \
    .rept 1                     \
      .rept 1000                \
        add  x5, x5, x7;        \
        sw   x6, 0(x5);         \
        sw   x6, 0(x5);         \
      .endr;                    \
     .endr; 


RVTEST_RV64U
RVTEST_CODE_BEGIN


#PFC_L1IR_TEST
PFC_L1IRM2_TEST
#PFC_L1DR_TEST
#PFC_L1DW_TEST
#PFC_L1DWB_TEST
#PFC_L2R_TEST
PFC_L2RB_TEST
#PFC_L2WB_TEST

GET_PFC
RVTEST_PASS

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
