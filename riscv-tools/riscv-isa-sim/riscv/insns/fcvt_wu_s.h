require_extension('F');
require_fp;
softfloat_roundingMode = RM;
WRITE_RD(reg_t(sext32(f32_to_ui32(f32(FRS1.data), RM, true))));
set_fp_exceptions;
