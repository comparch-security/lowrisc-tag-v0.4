// See LICENSE for license details.

#include "insn_template.h"

reg_t rv32_NAME(processor_t* p, insn_t insn, reg_t pc)
{
  int xlen = 32;
  reg_t npc = reg_t(sext_xlen(pc.data + insn_length(OPCODE)), pc.tag);
  #include "insns/NAME.h"
  trace_opcode(p, OPCODE, insn);
  return npc;
}

reg_t rv64_NAME(processor_t* p, insn_t insn, reg_t pc)
{
  int xlen = 64;
  reg_t npc = reg_t(sext_xlen(pc.data + insn_length(OPCODE)), pc.tag);
  #include "insns/NAME.h"
  trace_opcode(p, OPCODE, insn);
  return npc;
}
