require_extension('F');
require_fp;
WRITE_FRD(MMU.load_uint32(RS1.data + insn.i_imm()));
