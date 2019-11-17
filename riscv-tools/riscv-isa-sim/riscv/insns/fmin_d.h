require_extension('D');
require_fp;
WRITE_FRD(isNaNF64UI(FRS2.data) || f64_lt_quiet(f64(FRS1.data), f64(FRS2.data)) ? FRS1 : FRS2);
set_fp_exceptions;
