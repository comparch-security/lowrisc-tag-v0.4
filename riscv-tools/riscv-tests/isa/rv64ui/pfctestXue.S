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

#define L1DLINEW  6
#define L1DSETW   5
#define L2LINEW   6
#define L2SETW    7
#define L2BANKW   0

#define PFC_L1IR_TEST           \
    .rept 10000                 \
      nop;                      \
    .endr;                      
   

#define PFC_L1DR_TEST           \
    addi x5, x0, PFC_MEM;       \
    slli x5, x5, 28;            \
    .rept 1000                     \
      addi x5, x5, -1000;       \
      .set offset, 1000;        \
      .set offstep, 1;          \
      .rept 1000;               \
        offset=offset-offstep;  \
        sb x6, offset(x5);      \
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
    .set offsetw, L1DLINEW+L1DSETW;  \
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
    .set offsetw, L2LINEW+L2SETW;    \
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
    .set offsetw, L2LINEW+L2SETW;    \
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


RVTEST_RV64U
RVTEST_CODE_BEGIN


#PFC_L1IR_TEST
PFC_L1DR_TEST
#PFC_L1DW_TEST
#PFC_L1DWB_TEST
#PFC_L2R_TEST
#PFC_L2RB_TEST
#PFC_L2WB_TEST

GET_PFC
RVTEST_PASS

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
