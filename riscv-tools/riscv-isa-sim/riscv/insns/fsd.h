require_extension('D');
require_fp;
MMU.store_uint64(RS1.data + insn.s_imm(), FRS2);
