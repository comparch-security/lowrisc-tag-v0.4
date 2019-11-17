require_extension('F');
require_fp;
WRITE_FRD(isNaNF32UI(FRS2.data) || f32_le_quiet(f32(FRS2.data), f32(FRS1.data)) ? FRS1 : FRS2);
set_fp_exceptions;
