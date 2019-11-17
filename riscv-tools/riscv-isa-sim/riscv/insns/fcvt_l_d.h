require_extension('D');
require_rv64;
require_fp;
softfloat_roundingMode = RM;
WRITE_RD(reg_t(f64_to_i64(f64(FRS1.data), RM, true)));
set_fp_exceptions;
