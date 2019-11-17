require_extension('M');
if (xlen == 64)
  WRITE_RD(reg_t(mulh(RS1.data, RS2.data)));
else
  WRITE_RD(reg_t(sext32((sext32(RS1.data) * sext32(RS2.data)) >> 32)));
