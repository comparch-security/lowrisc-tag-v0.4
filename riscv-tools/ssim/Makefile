# See LICENSE for license details.

#--------------------------------------------------------------------
# global define
#--------------------------------------------------------------------

base_dir = $(abspath ..)
sim_dir = .
output_dir = $(sim_dir)/output

SPIKE_FLAGS = -p1 -m2048 -t2 --ic=64:4:64 --dc=64:4:64 --l2=256:8:64 --tt=64:8:64:1 --tm0=4:4:16:1
#SPIKE_FLAGS = -p1 -m2048 -t4 --ic=64:4:64 --dc=64:4:64 --l2=256:8:64 --utc=64:8:64:1

.PHONY: default

#--------------------------------------------------------------------
# Test cases
#--------------------------------------------------------------------
rv32ui = simple add addi and andi auipc beq bge bgeu blt bltu bne fence_i \
         j jal jalr lb lbu lh lhu lui lw or ori sb sh sw sll slli \
         slt slti sra srai srl srli sub xor xori \

rv32um = mul mulh mulhsu mulhu div divu rem remu \

rv32ua = amoadd_w amoand_w amoor_w amoxor_w amoswap_w amomax_w amomaxu_w amomin_w amominu_w \

rv32si = csr ma_fetch scall sbreak wfi \

rv32mi = csr mcsr dirty illegal ma_addr ma_fetch sbreak scall \

rv64ui = $(rv32ui) addw addiw ld lwu sd slliw sllw sltiu sltu sraiw sraw srliw srlw subw \

rv64um = $(rv32um) divuw divw mulw remuw remw \

rv64ua = amoadd_d amoand_d amoor_d amoxor_d amoswap_d amomax_d amomaxu_d amomin_d amominu_d \

rv64uf = ldst move fsgnj fcmp fcvt fcvt_w fclass fadd fdiv fmin fmadd structural \

rv64si = $(rv32si)

rv64mi = $(rv32mi)

bmarks = median multiply qsort towers vvadd mm dhrystone spmv mt-vvadd mt-matmul

#--------------------------------------------------------------------
asm_p_tests  += $(addprefix rv64ui-p-, $(rv64ui))
asm_p_tests  += $(addprefix rv64ui-p-, $(rv64um))
asm_p_tests  += $(addprefix rv64ui-p-, $(rv64ua))
asm_p_tests  += $(addprefix rv64uf-p-, $(rv64uf))
asm_p_tests  += $(addprefix rv64si-p-, $(rv64si))
asm_p_tests  += $(addprefix rv64mi-p-, $(rv64mi))

asm_v_tests  += $(addprefix rv64ui-v-, $(rv64ui))
asm_v_tests  += $(addprefix rv64ui-v-, $(rv64um))
asm_v_tests  += $(addprefix rv64ui-v-, $(rv64ua))
asm_v_tests  += $(addprefix rv64uf-v-, $(rv64uf))

bmarks_tests += $(bmarks)

#--------------------------------------------------------------------
riscv_test_asm_dir = $(base_dir)/riscv-tests/isa
riscv_test_bmarks_dir = $(base_dir)/riscv-tests/benchmarks

$(addprefix $(output_dir)/, $(addsuffix .riscv, $(asm_p_tests) $(asm_v_tests))):
	mkdir -p $(output_dir)
	$(MAKE) -C $(riscv_test_asm_dir) $(basename $(notdir $@))
	ln -fs $(riscv_test_asm_dir)/$(basename $(notdir $@)) $@

$(addprefix $(output_dir)/, $(addsuffix .riscv, $(bmarks_tests))):
	mkdir -p $(output_dir)
	$(MAKE) -C $(riscv_test_bmarks_dir) $(basename $(notdir $@))
	ln -fs $(riscv_test_asm_dir)/$(basename $(notdir $@)) $@

#--------------------------------------------------------------------
# Run (verilator)
#--------------------------------------------------------------------
asm_tests_out = $(foreach test, $(asm_p_tests) $(asm_pt_tests) $(asm_v_tests), $(output_dir)/$(test).out)
bmarks_out = $(foreach test, $(bmarks_tests), $(output_dir)/$(test).out)

$(output_dir)/%.out: $(output_dir)/%.riscv
	spike $(SPIKE_FLAGS) $< 2>&1 | spike-dasm > $@ &&  [ $$PIPESTATUS -eq 0 ]

.PRECIOUS: $(output_dir)/%.out

run-asm-tests: $(asm_tests_out)
run-bmarks-test: $(bmarks_out)

run: run-asm-tests run-bmarks-test

.PHONY: run-asm-tests run-bmarks-test run
junk += $(output_dir)

#--------------------------------------------------------------------
# clean up
#--------------------------------------------------------------------

clean:
	rm -rf $(junk)

.PHONY: clean
