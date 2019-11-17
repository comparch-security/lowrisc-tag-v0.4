require_extension('M');
require_rv64;
word_t lhs = zext32(RS1.data);
word_t rhs = zext32(RS2.data);
if(rhs == 0)
  WRITE_RD(reg_t(sext32(lhs)));
else
  WRITE_RD(reg_t(sext32(lhs % rhs)));
