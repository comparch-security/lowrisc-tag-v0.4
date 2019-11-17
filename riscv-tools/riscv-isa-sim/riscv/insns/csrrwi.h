int csr = validate_csr(insn.csr(), true);
word_t old = p->get_csr(csr);
p->set_csr(csr, insn.rs1());
WRITE_RD(reg_t(sext_xlen(old)));
