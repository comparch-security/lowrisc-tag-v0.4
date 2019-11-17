require_extension('C');
if (insn.rvc_rd() == 2) { // c.addi16sp
  require(insn.rvc_addi16sp_imm() != 0);
  WRITE_REG(X_SP, reg_t(sext_xlen(RVC_SP.data + insn.rvc_addi16sp_imm())));
} else {
  require(insn.rvc_rd() != 0);
  WRITE_RD(reg_t(insn.rvc_imm() << 12));
}
