require_extension('D');
require_fp;
WRITE_FRD(freg_t(FRS1.data ^ (FRS2.data & INT64_MIN)));
