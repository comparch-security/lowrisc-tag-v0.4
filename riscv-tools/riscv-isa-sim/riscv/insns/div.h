require_extension('M');
sword_t lhs = sext_xlen(RS1.data);
sword_t rhs = sext_xlen(RS2.data);
if(rhs == 0)
  WRITE_RD(reg_t(UINT64_MAX));
else if(lhs == INT64_MIN && rhs == -1)
  WRITE_RD(reg_t(lhs));
else
  WRITE_RD(reg_t(sext_xlen(lhs / rhs)));
