require_rv64;
WRITE_RD(MMU.load_int64(RS1.data + insn.i_imm()));
