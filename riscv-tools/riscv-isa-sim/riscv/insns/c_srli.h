require_extension('C');
require(insn.rvc_zimm() < xlen);
WRITE_RVC_RS1S(reg_t(sext_xlen(zext_xlen(RVC_RS1S.data) >> insn.rvc_zimm())));
