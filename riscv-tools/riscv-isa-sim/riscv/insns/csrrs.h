int csr = validate_csr(insn.csr(), insn.rs1() != 0);
word_t old = p->get_csr(csr);
p->set_csr(csr, old | RS1.data);
WRITE_RD(reg_t(sext_xlen(old)));
