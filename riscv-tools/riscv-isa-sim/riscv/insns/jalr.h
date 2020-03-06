if (insn.tag()  == 0)
    throw trap_tag_check_failed();
reg_t tmp = npc;
set_pc((RS1.data + insn.i_imm()) & ~word_t(1));
WRITE_RD(tmp);
