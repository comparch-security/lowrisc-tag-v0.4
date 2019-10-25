# See LICENSE for license details.

#*****************************************************************************
# store.S
#-----------------------------------------------------------------------------
#
# Test tagged store.
#

#define __TAG_MODE
#include "riscv_test.h"
#include "test_macros.h"

RVTEST_RV64U
RVTEST_CODE_BEGIN

        la   x10, tdat          ;
        
        li   x1, TMASK_LOAD_PROP ;
        csrw utagctrl, x1       ;
        li   x1, TMASK_STORE_PROP ;
        csrs utagctrl, x1       ;

# lb, lw, lh has no different in set tag
        TEST_CASE( 2, x5, 1,     \
        li   x1, 1              ;\
        tagw x1, x1             ;\
        sb   x1, 1(x10)         ;\
        nop                     ;\
        ld   x5, 0(x10)         ;\
        tagr x5, x5             ;\
        )

        TEST_CASE( 3, x5, 2,     \
        li   x1, 2              ;\
        tagw x1, x1             ;\
        sh   x1, 2(x10)         ;\
        nop                     ;\
        ld   x5, 0(x10)         ;\
        tagr x5, x5             ;\
        )

        TEST_CASE( 4, x5, 3,     \
        li   x1, 3              ;\
        tagw x1, x1             ;\
        sw   x1, 4(x10)         ;\
        nop                     ;\
        ld   x5, 0(x10)         ;\
        tagr x5, x5             ;\
        )

        TEST_CASE( 5, x5, 4,     \
        li   x1, 4              ;\
        tagw x1, x1             ;\
        sd   x1, 0(x10)         ;\
        nop                     ;\
        ld   x5, 0(x10)         ;\
        tagr x5, x5             ;\
        )

# Set a partial store tag propagation mask
        li   x1, 0x9
        slli x1, x1, TSHIM_STORE_PROP
        csrw utagctrl, x1
        li   x1, TMASK_LOAD_PROP
        csrs utagctrl, x1

# Ensure the propagation mask works
        TEST_CASE( 6, x5, 9,     \
        li   x1, 0xf            ;\
        tagw x1, x1             ;\
        sd   x1, 0(x10)         ;\
        ld   x5, 0(x10)         ;\
        tagr x5, x5             ;\
        )

  TEST_PASSFAIL

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

tdat:  .dword 0x0000ffff0f0f0f0f

RVTEST_DATA_END
