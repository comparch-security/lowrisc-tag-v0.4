require_extension('D');
require_fp;
softfloat_roundingMode = RM;
WRITE_FRD(freg_t(f64_mulAdd(f64(FRS1.data), f64(FRS2.data), f64(FRS3.data ^ (uint64_t)INT64_MIN)).v));
set_fp_exceptions;
