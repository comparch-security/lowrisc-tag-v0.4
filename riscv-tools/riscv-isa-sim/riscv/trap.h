// See LICENSE for license details.

#ifndef _RISCV_TRAP_H
#define _RISCV_TRAP_H

#include "decode.h"
#include <stdlib.h>

struct state_t;

class trap_t
{
 public:
  trap_t(word_t which) : which(which) {}
  virtual const char* name();
  virtual bool has_badaddr() { return false; }
  virtual word_t get_badaddr() { abort(); }
  word_t cause() { return which; }
 private:
  char _name[16];
  word_t which;
};

class mem_trap_t : public trap_t
{
 public:
  mem_trap_t(word_t which, word_t badaddr)
    : trap_t(which), badaddr(badaddr) {}
  bool has_badaddr() override { return true; }
  word_t get_badaddr() override { return badaddr; }
 private:
  word_t badaddr;
};

#define DECLARE_TRAP(n, x) class trap_##x : public trap_t { \
 public: \
  trap_##x() : trap_t(n) {} \
  const char* name() { return "trap_"#x; } \
};

#define DECLARE_MEM_TRAP(n, x) class trap_##x : public mem_trap_t { \
 public: \
  trap_##x(word_t badaddr) : mem_trap_t(n, badaddr) {} \
  const char* name() { return "trap_"#x; } \
};

DECLARE_MEM_TRAP(CAUSE_MISALIGNED_FETCH, instruction_address_misaligned)
DECLARE_MEM_TRAP(CAUSE_FAULT_FETCH, instruction_access_fault)
DECLARE_TRAP(CAUSE_ILLEGAL_INSTRUCTION, illegal_instruction)
DECLARE_TRAP(CAUSE_BREAKPOINT, breakpoint)
DECLARE_MEM_TRAP(CAUSE_MISALIGNED_LOAD, load_address_misaligned)
DECLARE_MEM_TRAP(CAUSE_MISALIGNED_STORE, store_address_misaligned)
DECLARE_MEM_TRAP(CAUSE_FAULT_LOAD, load_access_fault)
DECLARE_MEM_TRAP(CAUSE_FAULT_STORE, store_access_fault)
DECLARE_TRAP(CAUSE_USER_ECALL, user_ecall)
DECLARE_TRAP(CAUSE_SUPERVISOR_ECALL, supervisor_ecall)
DECLARE_TRAP(CAUSE_HYPERVISOR_ECALL, hypervisor_ecall)
DECLARE_TRAP(CAUSE_MACHINE_ECALL, machine_ecall)
DECLARE_TRAP(CAUSE_TAG_CHECK_FAIL, tag_check_failed)
DECLARE_MEM_TRAP(CAUSE_TAG_CHECK_FAIL, mem_tag_exception)

#endif
