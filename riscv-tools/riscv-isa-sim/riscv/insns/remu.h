require_extension('M');
word_t lhs = zext_xlen(RS1.data);
word_t rhs = zext_xlen(RS2.data);
if(rhs == 0)
  WRITE_RD(reg_t(sext_xlen(RS1.data)));
else
  WRITE_RD(reg_t(sext_xlen(lhs % rhs)));
