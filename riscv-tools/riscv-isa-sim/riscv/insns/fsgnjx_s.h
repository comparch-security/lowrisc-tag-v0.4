require_extension('F');
require_fp;
WRITE_FRD(freg_t(FRS1.data ^ (FRS2.data & (uint32_t)INT32_MIN)));
