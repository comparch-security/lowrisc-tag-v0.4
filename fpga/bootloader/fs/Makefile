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
FPGA_DIR ?= ../../board/nexys4_ddr/
GEN_DIR = $(FPGA_DIR)/generated-src
PAYLOAD_DIR ?= ../payload
RISCV_PREFIX=riscv64-unknown-elf-
RISCV_GCC = $(RISCV_PREFIX)gcc
RISCV_GCC_OPTS = -MMD -MP -Wall -Werror -D__NO_INLINE__ -DENABLE_ATOMICS -mcmodel=medany -O2 -std=gnu99 -Wno-unused -Wno-attributes -fno-delete-null-pointer-checks  -I. -I.. -I$(DRIVER_DIR) -I$(GEN_DIR)
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


S_OBJS = \
	mentry.o \
	entry.o \
	sbi_entry.o \

	
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

junk += \
	$(C_OBJS) $(S_OBJS) $(DRIVER_OBJS) \
	$(patsubst %.o, %.d, $(C_OBJS)) \
	$(patsubst %.o, %.d, $(S_OBJS)) \
	$(patsubst %.o, %.d, $(DRIVER_OBJS)) \

#--------------------------------------------------------------------
# Building Targets
#--------------------------------------------------------------------

$(C_OBJS): %.o: %.c $(HEADERS)
	$(RISCV_GCC) $(RISCV_GCC_OPTS) -c $< -o $@

$(S_OBJS): %.o: %.S $(HEADERS)
	$(RISCV_GCC) $(RISCV_GCC_OPTS) -c $< -o $@

$(DRIVER_OBJS): %.o:$(DRIVER_DIR)/%.c $(DRIVER_HEADERS)
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
