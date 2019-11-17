require_extension('F');
require_rv64;
require_fp;
softfloat_roundingMode = RM;
WRITE_FRD(freg_t(i64_to_f32(RS1.data).v));
set_fp_exceptions;
