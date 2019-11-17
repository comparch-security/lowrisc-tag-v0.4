require(SHAMT < xlen);
WRITE_RD(reg_t(sext_xlen(zext_xlen(RS1.data) >> SHAMT)));
