require_extension('D');
require_fp;
softfloat_roundingMode = RM;
WRITE_RD(reg_t(sext32(f64_to_ui32(f64(FRS1.data), RM, true))));
set_fp_exceptions;
