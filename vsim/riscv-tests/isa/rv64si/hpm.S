# See LICENSE for license details.

#*****************************************************************************
# hpm.S
#-----------------------------------------------------------------------------
#
# test for rdtime rdcycle and rdinstret instructions
#

#include "riscv_test.h"
#include "test_macros.h"

        csrw mucounteren, 7     ;

RVTEST_RV64U
RVTEST_CODE_BEGIN

  TEST_CASE(2, a1, 0, \
        .align 6                ; \
        rdtime a1               ; \
        rdtime a2               ; \
        rdtime a3               ; \
        sub  a1, a2, a1         ; \
        sub  a2, a3, a2         ; \
        sub  a1, a2, a1         ; \
        )

  TEST_CASE(3, a1, 0, \
        .align 6                ; \
        rdcycle a1              ; \
        rdcycle a2              ; \
        rdcycle a3              ; \
        sub  a1, a2, a1         ; \
        sub  a2, a3, a2         ; \
        sub  a1, a2, a1         ; \
        )

  TEST_CASE(4, a1, 1, \
        .align 6                ; \
        rdinstret a1            ; \
        rdinstret a2            ; \
        sub  a1, a2, a1         ; \
        )

  TEST_PASSFAIL

RVTEST_PASS

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
