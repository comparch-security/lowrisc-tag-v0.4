require_extension('F');
require_fp;
WRITE_FRD(isNaNF32UI(FRS2.data) || f32_lt_quiet(f32(FRS1.data), f32(FRS2.data)) ? FRS1 : FRS2);
set_fp_exceptions;
