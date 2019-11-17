require_extension('C');
require(insn.rvc_rs1() != 0);
set_pc(RVC_RS1.data & ~word_t(1));
