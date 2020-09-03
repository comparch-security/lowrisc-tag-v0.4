#=======================================================================
# Makefile for Berkeley bootloader (BBL)
#-----------------------------------------------------------------------
# See LICENSE for license details.

# check RISCV environment variable
ifndef RISCV
$(error Please set environment variable RISCV. Please take a look at README)
endif

default: all

TARGET = bbl

all: $(TARGET)

junk += $(TARGET) $(TARGET).hex

.PHONY: all

#--------------------------------------------------------------------
# Build rules
#--------------------------------------------------------------------

DRIVER_DIR = ../driver
SOFTFLOAT_DIR = ../softfloat
FPGA_DIR ?= ../../board/nexys4_ddr/
GEN_DIR = $(FPGA_DIR)/generated-src
PAYLOAD_DIR ?= ../payload
RISCV_PREFIX=riscv64-unknown-elf-
RISCV_GCC = $(RISCV_PREFIX)gcc
RISCV_GCC_OPTS = -MMD -MP -Wall -Werror -D__NO_INLINE__ -DENABLE_ATOMICS -mcmodel=medany -O2 -std=gnu99 -Wno-unused -Wno-attributes -fno-delete-null-pointer-checks  -I. -I.. -I$(DRIVER_DIR) -I$(SOFTFLOAT_DIR) -I$(GEN_DIR)
RISCV_LINK_OPTS = -nostartfiles -nostdlib -static -T bbl.lds -lgcc
RISCV_DUMP = $(RISCV_PREFIX)objdump
RISCV_DUMP_OPTS = -D -S -l


#--------------------------------------------------------------------
# Objects
#--------------------------------------------------------------------
C_OBJS = \
	bbl.o \
	mtrap.o \
	string.o \
	snprintf.o \
	minit.o \
	configstring.o \
	file_test.o \
	console.o \
	file.o \
	mmap.o \
	syscall.o \
	handlers.o \
	sbi_impl.o \
	frontend.o \
	remove_htif.o \
	elf.o \
	pfc.o \
	# emulation.o \
	# fp_emulation.o \
	# fp_ldst.o \
	# misaligned_ldst.o \
	# muldiv_emulation.o \


S_OBJS = \
	mentry.o \
	entry.o \
	sbi_entry.o \
	fp_asm.o \

	
HEADERS += \
	$(wildcard *.h) \
	$(GEN_DIR)/dev_map.h \

DRIVER_OBJS = \
	uart.o \
	spi.o \
	diskio.o \
	ff.o \
	unicode.o \

DRIVER_HEADERS += \
	$(wildcard $(DRIVER_DIR)/*.h) \

SOFTFLOAT_C_SRCS = \
	f32_add.c                      \
	f32_div.c                      \
	f32_eq.c                       \
	f32_eq_signaling.c             \
	f32_isSignalingNaN.c           \
	f32_le.c                       \
	f32_le_quiet.c                 \
	f32_lt.c                       \
	f32_lt_quiet.c                 \
	f32_mulAdd.c                   \
	f32_mul.c                      \
	f32_rem.c                      \
	f32_roundToInt.c               \
	f32_sqrt.c                     \
	f32_sub.c                      \
	f32_to_f64.c                   \
	f32_to_i32.c                   \
	f32_to_i32_r_minMag.c          \
	f32_to_i64.c                   \
	f32_to_i64_r_minMag.c          \
	f32_to_ui32.c                  \
	f32_to_ui32_r_minMag.c         \
	f32_to_ui64.c                  \
	f32_to_ui64_r_minMag.c         \
	f32_classify.c                 \
	f64_add.c                      \
	f64_div.c                      \
	f64_eq.c                       \
	f64_eq_signaling.c             \
	f64_isSignalingNaN.c           \
	f64_le.c                       \
	f64_le_quiet.c                 \
	f64_lt.c                       \
	f64_lt_quiet.c                 \
	f64_mulAdd.c                   \
	f64_mul.c                      \
	f64_rem.c                      \
	f64_roundToInt.c               \
	f64_sqrt.c                     \
	f64_sub.c                      \
	f64_to_f32.c                   \
	f64_to_i32.c                   \
	f64_to_i32_r_minMag.c          \
	f64_to_i64.c                   \
	f64_to_i64_r_minMag.c          \
	f64_to_ui32.c                  \
	f64_to_ui32_r_minMag.c         \
	f64_to_ui64.c                  \
	f64_to_ui64_r_minMag.c         \
	f64_classify.c                 \
	i32_to_f32.c                   \
	i32_to_f64.c                   \
	i64_to_f32.c                   \
	i64_to_f64.c                   \
	s_add128.c                     \
	s_add192.c                     \
	s_addMagsF32.c                 \
	s_addMagsF64.c                 \
	s_countLeadingZeros32.c        \
	s_countLeadingZeros64.c        \
	s_countLeadingZeros8.c         \
	s_eq128.c                      \
	s_estimateDiv128To64.c         \
	s_estimateSqrt32.c             \
	s_le128.c                      \
	s_lt128.c                      \
	s_mul128By64To192.c            \
	s_mul128To256.c                \
	s_mul64To128.c                 \
	s_mulAddF32.c                  \
	s_mulAddF64.c                  \
	s_normRoundPackToF32.c         \
	s_normRoundPackToF64.c         \
	s_normSubnormalF32Sig.c        \
	s_normSubnormalF64Sig.c        \
	s_roundPackToF32.c             \
	s_roundPackToF64.c             \
	s_roundPackToI32.c             \
	s_roundPackToI64.c             \
	s_roundPackToUI32.c            \
	s_roundPackToUI64.c            \
	s_shift128ExtraRightJam.c      \
	s_shift128RightJam.c           \
	s_shift32RightJam.c            \
	s_shift64ExtraRightJam.c       \
	s_shift64RightJam.c            \
	s_shortShift128ExtraRightJam.c \
	s_shortShift128Left.c          \
	s_shortShift128Right.c         \
	s_shortShift192Left.c          \
	s_shortShift32Right1Jam.c      \
	s_shortShift64ExtraRightJam.c  \
	s_shortShift64RightJam.c       \
	s_sub128.c                     \
	s_sub192.c                     \
	s_subMagsF32.c                 \
	s_subMagsF64.c                 \
	ui32_to_f32.c                  \
	ui32_to_f64.c                  \
	ui64_to_f32.c                  \
	ui64_to_f64.c                  \
	s_commonNaNToF64UI.c   \
	s_f32UIToCommonNaN.c   \
	s_f64UIToCommonNaN.c   \
	s_propagateNaNF32UI.c  \
	s_propagateNaNF64UI.c  \


SOFTFLOAT_HEADERS += \
	$(wildcard $(SOFTFLOAT_DIR)/*.h) \

SOFTFLOAT_OBJS = \
	$(patsubst %.c, %.o, $(SOFTFLOAT_C_SRCS)) \

junk += \
	$(C_OBJS) $(S_OBJS) $(DRIVER_OBJS) $(SOFTFLOAT_OBJS) \
	$(patsubst %.o, %.d, $(C_OBJS)) \
	$(patsubst %.o, %.d, $(S_OBJS)) \
	$(patsubst %.o, %.d, $(DRIVER_OBJS)) \
	$(patsubst %.o, %.d, $(SOFTFLOAT_OBJS)) \

#--------------------------------------------------------------------
# Building Targets
#--------------------------------------------------------------------

$(C_OBJS): %.o: %.c $(HEADERS) 
	$(RISCV_GCC) $(RISCV_GCC_OPTS) -c $< -o $@ 

$(S_OBJS): %.o: %.S $(HEADERS)
	$(RISCV_GCC) $(RISCV_GCC_OPTS) -c $< -o $@

$(DRIVER_OBJS): %.o:$(DRIVER_DIR)/%.c $(DRIVER_HEADERS)
	$(RISCV_GCC) $(RISCV_GCC_OPTS) -c $< -o $@

$(SOFTFLOAT_OBJS): %.o:$(SOFTFLOAT_DIR)/%.c $(SOFTFLOAT_HEADERS) $(HEADERS)
	$(RISCV_GCC) $(RISCV_GCC_OPTS) -c $< -o $@

$(TARGET): $(C_OBJS) $(S_OBJS) $(DRIVER_OBJS) 
	$(RISCV_GCC) -o $@ $(C_OBJS) $(S_OBJS) $(DRIVER_OBJS) $(RISCV_LINK_OPTS)
	cp $(TARGET) ../../board/nexys4_ddr/

dump: $(TARGET).dump
$(TARGET).dump: $(TARGET)
	$(RISCV_DUMP) $(RISCV_DUMP_OPTS) $< > $@

hex: $(TARGET).hex
$(TARGET).hex: $(TARGET)
	elf2hex 16 32768 $< > $@

payload: 
	$(MAKE) -C $(PAYLOAD_DIR)

.PHONY: dump payload

#--------------------------------------------------------------------
# clean up
#--------------------------------------------------------------------

clean:
	rm -rf $(junk)

.PHONY: clean
