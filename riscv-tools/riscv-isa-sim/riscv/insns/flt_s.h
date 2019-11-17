require_extension('F');
require_fp;
WRITE_RD(reg_t(f32_lt(f32(FRS1.data), f32(FRS2.data))));
set_fp_exceptions;
