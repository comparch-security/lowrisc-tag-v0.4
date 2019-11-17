require_extension('C');
if (xlen == 32) {
  reg_t tmp = npc;
  set_pc(pc.data + insn.rvc_j_imm());
  WRITE_REG(X_RA, tmp);
} else { // c.addiw
  require(insn.rvc_rd() != 0);
  WRITE_RD(reg_t(sext32(RVC_RS1.data + insn.rvc_imm())));
}
