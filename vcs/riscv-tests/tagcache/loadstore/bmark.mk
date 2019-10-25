#=======================================================================
# UCB CS250 Makefile fragment for benchmarks
#-----------------------------------------------------------------------
#
# Each benchmark directory should have its own fragment which
# essentially lists what the source files are and how to link them
# into an riscv and/or host executable. All variables should include
# the benchmark name as a prefix so that they are unique.
#

loadstore_c_src = \
	loadstore_main.c \
	syscalls.c \

loadstore_riscv_src = \
	crt.S \

loadstore_c_objs     = $(patsubst %.c, %.o, $(loadstore_c_src))
loadstore_riscv_objs = $(patsubst %.S, %.o, $(loadstore_riscv_src))

loadstore_host_bin = loadstore.host
$(loadstore_host_bin) : $(loadstore_c_src)
	$(HOST_COMP) $^ -o $(loadstore_host_bin)

loadstore_riscv_bin = loadstore.riscv
$(loadstore_riscv_bin) : $(loadstore_c_objs) $(loadstore_riscv_objs)
	$(RISCV_LINK) $(loadstore_c_objs) $(loadstore_riscv_objs) -o $(loadstore_riscv_bin) $(RISCV_LINK_OPTS)

junk += $(loadstore_c_objs) $(loadstore_riscv_objs) \
        $(loadstore_host_bin) $(loadstore_riscv_bin)
