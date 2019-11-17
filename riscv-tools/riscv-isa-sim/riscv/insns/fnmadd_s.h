require_extension('F');
require_fp;
softfloat_roundingMode = RM;
WRITE_FRD(freg_t(f32_mulAdd(f32(FRS1.data ^ (uint32_t)INT32_MIN), f32(FRS2.data), f32(FRS3.data ^ (uint32_t)INT32_MIN)).v));
set_fp_exceptions;
