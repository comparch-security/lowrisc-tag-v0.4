require_extension('C');
require(insn.rvc_addi4spn_imm() != 0);
WRITE_RVC_RS2S(reg_t(sext_xlen(RVC_SP.data + insn.rvc_addi4spn_imm())));
