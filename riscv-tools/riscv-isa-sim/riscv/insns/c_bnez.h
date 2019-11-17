require_extension('C');
if (RVC_RS1S.data != 0)
  set_pc(pc.data + insn.rvc_b_imm());
