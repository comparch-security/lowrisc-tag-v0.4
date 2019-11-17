require_extension('C');
WRITE_RD(reg_t(sext_xlen(RVC_RS1.data + insn.rvc_imm())));
