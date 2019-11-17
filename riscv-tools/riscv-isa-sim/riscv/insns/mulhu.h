require_extension('M');
if (xlen == 64)
  WRITE_RD(reg_t(mulhu(RS1.data, RS2.data)));
else
  WRITE_RD(reg_t(sext32(((uint64_t)(uint32_t)RS1.data * (uint64_t)(uint32_t)RS2.data) >> 32)));
