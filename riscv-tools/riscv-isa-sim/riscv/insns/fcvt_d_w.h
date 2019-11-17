require_extension('D');
require_fp;
softfloat_roundingMode = RM;
WRITE_FRD(freg_t(i32_to_f64((int32_t)RS1.data).v));
set_fp_exceptions;
