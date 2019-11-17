require_extension('D');
require_fp;
softfloat_roundingMode = RM;
WRITE_FRD(freg_t(f32_to_f64(f32(FRS1.data)).v));
set_fp_exceptions;
