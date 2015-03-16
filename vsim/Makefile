#=======================================================================
# Makefile for Verilog simulation w/ VCS
#-----------------------------------------------------------------------
# Yunsup Lee (yunsup@cs.berkeley.edu)
#
# This makefile will build a rtl simulator and run various tests to
# verify proper functionality.
#

default: all

base_dir = $(abspath ..)
generated_dir = $(abspath ./generated-src)
mem_gen = $(base_dir)/vsim/vlsi_mem_gen
sim_dir = .
output_dir = $(sim_dir)/output
tag_output_dir = $(sim_dir)/tag_output

BACKEND ?= lowrisc_chip.LowRISCChipBackend
CONFIG ?= DefaultVLSIConfig

include $(base_dir)/Makefrag
include $(sim_dir)/Makefrag
include $(base_dir)/vsim/Makefrag-sim

all: $(simv)
debug: $(simv_debug)

clean:
	rm -rf $(junk) simv* csrc *.key DVE* *.h *.a *.daidir $(generated_dir)

.PHONY: default all debug clean