require_extension('C');
require(insn.rvc_rs2() != 0);
WRITE_RD(reg_t(sext_xlen(RVC_RS1.data + RVC_RS2.data)));
