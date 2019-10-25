# See LICENSE for license details.

#*****************************************************************************
# load.S
#-----------------------------------------------------------------------------
#
# Test tagged load.
#

#define __TAG_MODE
#include "riscv_test.h"
#include "test_macros.h"

RVTEST_RV64U
RVTEST_CODE_BEGIN

        la   x10, tdat          ;

# initially all tag masks disabled
# no tag is read from memory
        TEST_CASE( 2, x5, 0x0000ffff0f0f0f0f,\
        ld x5, 0(x10)           ;\
        )

        TEST_CASE( 3, x5, 0x0,   \
        tagr x5, x5             ;\
        )

        li   x1, TMASK_LOAD_PROP ;
        csrw utagctrl, x1       ;
        li   x1, TMASK_STORE_PROP ;
        csrs utagctrl, x1       ;

# set a tag to memory
        TEST_CASE( 4, x5, 2,     \
        li   x1, 2              ;\
        tagw x1, x1             ;\
        sd   x1, 0(x10)         ;\
        ld   x5, 0(x10)         ;\
        tagr x5, x5             ;\
        )

# set a tag to memory, bypass from s2
        TEST_CASE( 5, x5, 3,     \
        li   x1, 3              ;\
.align 6                        ;\
        tagw x1, x1             ;\
        sd   x1, 0(x10)         ;\
        ld   x5, 0(x10)         ;\
        tagr x5, x5             ;\
        )

# set a tag to memory, bypass from s3
        TEST_CASE( 6, x5, 1,     \
        li   x1, 1              ;\
        tagw x1, x1             ;\
.align 6                        ;\
        sd   x1, 0(x10)         ;\
        nop                     ;\
        ld   x5, 0(x10)         ;\
        tagr x5, x5             ;\
        )

# set a tag to memory, bypass from s4
        TEST_CASE( 7, x5, 2,     \
        li   x1, 2              ;\
        tagw x1, x1             ;\
.align 6                        ;\
        sd   x1, 0(x10)         ;\
        nop                     ;\
        nop                     ;\
        ld   x5, 0(x10)         ;\
        tagr x5, x5             ;\
        )

# disable store tag propagation
        li   x1, TMASK_STORE_PROP ;
        csrc utagctrl, x1       ;

# load again to make sure it is not just forwarding
        TEST_CASE( 8, x5, 2,     \
        ld   x1, 0(x10)         ;\
        tagr x5, x1             ;\
        )

# lb, lw, lh has no different in load tag
        li   TESTNUM, 9         ;
        li   x4, 2              ;
        lb   x1, 3(x10)         ;
        tagr x5, x1             ;
        bne  x4, x5, fail       ;

        li   TESTNUM, 10        ;
        lh   x1, 2(x10)         ;
        tagr x5, x1             ;
        bne  x4, x5, fail       ;

        li   TESTNUM, 11        ;
        lw   x1, 4(x10)         ;
        tagr x5, x1             ;
        bne  x4, x5, fail       ;

# Set a partial load tag propagation mask
        li   x1, 0x9
        slli x1, x1, TSHIM_LOAD_PROP
        csrw utagctrl, x1
        li   x1, TMASK_STORE_PROP
        csrs utagctrl, x1

# Ensure the propagation mask works
        TEST_CASE( 12, x5, 9,    \
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
