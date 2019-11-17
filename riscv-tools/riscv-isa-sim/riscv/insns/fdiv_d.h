require_extension('D');
require_fp;
softfloat_roundingMode = RM;
WRITE_FRD(freg_t(f64_div(f64(FRS1.data), f64(FRS2.data)).v));
set_fp_exceptions;
