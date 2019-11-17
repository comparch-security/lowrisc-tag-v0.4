require_extension('F');
require_fp;
softfloat_roundingMode = RM;
WRITE_FRD(freg_t(f32_sqrt(f32(FRS1.data)).v));
set_fp_exceptions;
