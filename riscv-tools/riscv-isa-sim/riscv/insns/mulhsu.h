require_extension('M');
if (xlen == 64)
  WRITE_RD(reg_t(mulhsu(RS1.data, RS2.data)));
else
  WRITE_RD(reg_t(sext32((sext32(RS1.data) * word_t((uint32_t)RS2.data)) >> 32)));
