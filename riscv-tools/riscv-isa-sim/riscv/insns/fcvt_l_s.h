require_extension('F');
require_rv64;
require_fp;
softfloat_roundingMode = RM;
WRITE_RD(reg_t(f32_to_i64(f32(FRS1.data), RM, true)));
set_fp_exceptions;
