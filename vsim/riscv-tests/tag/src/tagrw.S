# See LICENSE for license details.

#*****************************************************************************
# tagrw.S
#-----------------------------------------------------------------------------
#
# Test tagr/tagw instruction.
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

# set x1 tag
        TEST_CASE( 4, x5, 0x1,   \
        li   x1, 0x1            ;\
        tagw x1, x1             ;\
        tagr x5, x1             ;\
        )

        TEST_CASE( 5, x5, 0x2,   \
        li   x2, 0x2            ;\
        tagw x2, x2             ;\
        tagr x5, x2             ;\
        )

        TEST_CASE( 6, x5, 0x4,   \
        li   x3, 0x4            ;\
        tagw x3, x3             ;\
        tagr x5, x3             ;\
        )

        TEST_CASE( 7, x5, 0x8,   \
        li   x4, 0x8            ;\
        tagw x4, x4             ;\
        tagr x5, x4             ;\
        )

        TEST_CASE( 8, x5, 0x1,   \
        addi x5, x1, 0          ;\
        )

        TEST_CASE( 9, x5, 0x2,   \
        addi x5, x2, 0          ;\
        )

        TEST_CASE( 11, x5, 0x4,  \
        addi x5, x3, 0          ;\
        )

        TEST_CASE( 12, x5, 0x8,  \
        addi x5, x4, 0          ;\
        )

# bypass tests
        TEST_CASE(13, x5, 0x1,   \
        li   x4, 0x1            ;\
.align 6                        ;\
        tagw x3, x4             ;\
        tagr x5, x3             ;\
        )

        TEST_CASE(14, x5, 0x2,   \
        li   x4, 0x2            ;\
.align 6                        ;\
        tagw x3, x4             ;\
        nop                     ;\
        tagr x4, x3             ;\
        nop                     ;\
        addi x5, x4, 0          ;\
        )

        TEST_CASE(15, x5, 0x3,   \
        li   x4, 0x3            ;\
.align 6                        ;\
        tagw x3, x4             ;\
        nop                     ;\
        nop                     ;\
        tagr x4, x3             ;\
        nop                     ;\
        nop                     ;\
        addi x5, x4, 0          ;\
        )

        TEST_CASE(16, x5, 0x4,   \
        li   x4, 0x4            ;\
.align 6                        ;\
        tagw x3, x4             ;\
        nop                     ;\
        nop                     ;\
        nop                     ;\
        tagr x4, x3             ;\
        nop                     ;\
        nop                     ;\
        nop                     ;\
        addi x5, x4, 0          ;\
        )

        TEST_CASE(17, x5, 0x5,   \
        li   x4, 0x1            ;\
        li   x3, 0x5            ;\
.align 6                        ;\
        tagw x3, x4             ;\
        nop                     ;\
        addi x5, x3, 0          ;\
        )

# Set tag wider than TAG_BITS
        TEST_CASE( 18, x5, 0xf,   \
        li   x1, 0xff           ;\
        tagw x1, x1             ;\
        tagr x5, x1             ;\
        )



  TEST_PASSFAIL

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
