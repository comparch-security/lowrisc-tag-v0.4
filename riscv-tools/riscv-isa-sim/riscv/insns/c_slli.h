require_extension('C');
require(insn.rvc_zimm() < xlen);
WRITE_RD(reg_t(sext_xlen(RVC_RS1.data << insn.rvc_zimm())));
