// See LICENSE for license details.

#include "bbl.h"
#include "syscall.h"
#include "vm.h"

static void handle_illegal_instruction(trapframe_t* tf)
{
  tf->insn = *(uint16_t*)tf->epc;
  // printk("illegal insn trap in S-mode. insn = %#lx\n",tf->insn);
  int len = insn_len(tf->insn);
  if (len == 4)
    tf->insn |= ((uint32_t)*(uint16_t*)(tf->epc + 2) << 16);
  else
    kassert(len == 2);

  // supply 0 for unimplemented uarch counters
  if ((tf->insn & (MASK_CSRRS | 0xcc0U<<20)) == (MATCH_CSRRS | 0xcc0U<<20)) {
    tf->gpr[(tf->insn >> 7) & 0x1f] = 0;
    tf->epc += 4;
    return;
  }

  dump_tf(tf);
  panic("An illegal instruction was executed!");
}

static void handle_breakpoint(trapframe_t* tf)
{
  dump_tf(tf);
  printk("Breakpoint!\n");
  tf->epc += 4;
}

static void handle_misaligned_fetch(trapframe_t* tf)
{
  dump_tf(tf);
  panic("Misaligned instruction access!");
}

static void handle_misaligned_store(trapframe_t* tf)
{
  dump_tf(tf);
  panic("Misaligned AMO!");
}

static void segfault(trapframe_t* tf, uintptr_t addr, const char* type)
{
  dump_tf(tf);
  const char* who = (tf->status & SSTATUS_SPP) ? "Kernel" : "User";
  panic("%s %s segfault @ %p", who, type, addr);
}

static void handle_fault_fetch(trapframe_t* tf)
{
  if (handle_page_fault(tf->badvaddr, PROT_EXEC) != 0)
    segfault(tf, tf->badvaddr, "fetch");
}

static void handle_fault_load(trapframe_t* tf)
{
  if (handle_page_fault(tf->badvaddr, PROT_READ) != 0)
    segfault(tf, tf->badvaddr, "load");
}

static void handle_fault_store(trapframe_t* tf)
{
  if (handle_page_fault(tf->badvaddr, PROT_WRITE) != 0)
    segfault(tf, tf->badvaddr, "store");
}

static void handle_syscall(trapframe_t* tf)
{
  tf->gpr[10] = do_syscall(tf->gpr[10], tf->gpr[11], tf->gpr[12], tf->gpr[13],
                           tf->gpr[14], tf->gpr[15], tf->gpr[17]);
  tf->epc += 4;
}

static void handle_interrupt(trapframe_t* tf)
{
  clear_csr(sip, SIP_SSIP);
  printk("interrupt caught in S mode. mcause is %p, mepc is %p\n",tf->cause,tf->epc);
  if (tf->cause == ((1lu<<63)|IRQ_HOST)){
    printk("we caught a rtc2 interrupt. terminate the program.\n");
    do_syscall(0,0,0,0,0,0,SYS_exit);
  }
}

static void handle_tag_check_failure(trapframe_t* tf)
{
  dump_tf(tf);
  panic("Tag check failure @ %p",tf->epc);
  /*printk("Tag failure epc %p eins %p ra %p rins %p\n",tf->epc,*((int*)tf->epc),tf->gpr[1],*((int*)tf->gpr[1])); //get faulting insn, if it wasn't a fetch-related trap
  
  #define set_tagged_val(dst, val, tag) \
  asm volatile ("tagw %0, %1; sw %0, 0(%2); tagw %0, zero;" : : "r" (val), "r" (tag), "r" (dst))
  set_tagged_val(&(tf->gpr[1]),tf->gpr[1],15);*/
}

static void handle_supervisorcall(trapframe_t* tf)
{
  while (1);
}

void handle_trap(trapframe_t* tf)
{
  // printk("trapped.\n");

  if ((intptr_t)tf->cause < 0)
    return handle_interrupt(tf);

  typedef void (*trap_handler)(trapframe_t*);

  const static trap_handler trap_handlers[] = {
    [CAUSE_MISALIGNED_FETCH] = handle_misaligned_fetch,
    [CAUSE_FAULT_FETCH] = handle_fault_fetch,
    [CAUSE_ILLEGAL_INSTRUCTION] = handle_illegal_instruction,
    [CAUSE_USER_ECALL] = handle_syscall,
    [CAUSE_SUPERVISOR_ECALL] = handle_supervisorcall,
    [CAUSE_BREAKPOINT] = handle_breakpoint,
    [CAUSE_MISALIGNED_STORE] = handle_misaligned_store,
    [CAUSE_FAULT_LOAD] = handle_fault_load,
    [CAUSE_FAULT_STORE] = handle_fault_store,
    [CAUSE_TAG_CHECK_FAIL] = handle_tag_check_failure,
  };

  kassert(tf->cause < ARRAY_SIZE(trap_handlers) && trap_handlers[tf->cause]);

  trap_handlers[tf->cause](tf);
  // printk("trap handled.\n");
}
