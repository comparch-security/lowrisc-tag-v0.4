require_extension('F');
require_fp;
softfloat_roundingMode = RM;
WRITE_FRD(freg_t(ui32_to_f32((uint32_t)RS1.data).v));
set_fp_exceptions;
