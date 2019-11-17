require_rv64;
WRITE_RD(MMU.load_uint32(RS1.data + insn.i_imm()));
