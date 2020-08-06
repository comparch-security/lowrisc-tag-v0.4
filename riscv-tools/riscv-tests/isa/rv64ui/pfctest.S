#include "riscv_test.h"
#include "test_macros.h"

#define PFC_L1DR_TEST           \
     la x5,pfc_base;             \
     .rept 1;                  \
       .set offset, 0;        \
       .set offstep, 1;          \
       .rept 1000;               \
         offset=offset+offstep;  \
         lb x6, offset(x5);      \
       .endr;                    \
       addi x5, x5, 1000;       \
      .endr;

RVTEST_RV64U
RVTEST_CODE_BEGIN

        PFC_L1DR_TEST
        # la x5, pfc_base;
        # lb x6, 0 (x5);

RVTEST_PASS

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

pfc_base:
.rept 100;
  .byte 0;
.endr;

RVTEST_DATA_END

