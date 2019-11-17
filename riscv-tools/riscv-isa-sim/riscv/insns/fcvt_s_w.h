require_extension('F');
require_fp;
softfloat_roundingMode = RM;
WRITE_FRD(freg_t(i32_to_f32((int32_t)RS1.data).v));
set_fp_exceptions;
