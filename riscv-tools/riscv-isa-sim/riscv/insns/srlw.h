require_rv64;
WRITE_RD(reg_t(sext32((uint32_t)RS1.data >> (RS2.data & 0x1F))));
