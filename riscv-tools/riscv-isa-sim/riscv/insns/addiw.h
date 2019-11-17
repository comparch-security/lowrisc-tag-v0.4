require_rv64;
WRITE_RD(reg_t(sext32(insn.i_imm() + RS1.data)));
