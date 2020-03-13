if (STATE.prv <= PRV_U && insn.tag_bits()  == 0)
    throw trap_tag_check_failed();
// if (STATE.prv <= PRV_S ) {
//     printf("Tag:%lu, ",insn.tag_bits());
//     printf("PC: %p\n",STATE.pc);
// }
reg_t tmp = npc;
set_pc((RS1.data + insn.i_imm()) & ~word_t(1));
WRITE_RD(tmp);
