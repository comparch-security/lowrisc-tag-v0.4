require_rv64;
WRITE_RD(reg_t(sext32(int32_t(RS1.data) >> SHAMT)));
