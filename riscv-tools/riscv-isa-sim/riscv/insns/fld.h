require_extension('D');
require_fp;
WRITE_FRD(MMU.load_int64(RS1.data + insn.i_imm()));
