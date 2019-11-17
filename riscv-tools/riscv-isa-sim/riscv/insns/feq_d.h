require_extension('D');
require_fp;
WRITE_RD(reg_t(f64_eq(f64(FRS1.data), f64(FRS2.data))));
set_fp_exceptions;
