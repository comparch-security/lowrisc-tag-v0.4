require_extension('M');
require_rv64;
sword_t lhs = sext32(RS1.data);
sword_t rhs = sext32(RS2.data);
if(rhs == 0)
  WRITE_RD(reg_t(lhs));
else
  WRITE_RD(reg_t(sext32(lhs % rhs)));
