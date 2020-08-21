#include "encoding.h"
#include "mcall.h"

  .section .sbi,"ax",@progbits
  .option norvc
  .align RISCV_PGSHIFT
  .globl sbi_base
sbi_base:

  # We no longer map this page into the highest virtual address space
  # because we have introduced tagmem into the highest address of memory.
  # In bblpk(and cachepfc) we call those sbi interface directly from the physical 
  # address (which has already been mapped into the page table for S-Mode)
  .skip RISCV_PGSIZE - 2048

  # hart_id
  .align 4
  .globl sbi_hart_id; 
sbi_hart_id:
  li a7, MCALL_HART_ID
  ecall
  ret

  # num_harts
  .align 4
  .globl sbi_num_harts; 
sbi_num_harts:
  lw a0, num_harts
  ret

  # query_memory
  .align 4
  .globl sbi_query_memory; 
sbi_query_memory:
  tail __sbi_query_memory

  # console_putchar
  .align 4
.globl sbi_console_putchar; 
sbi_console_putchar:
  li a7, MCALL_CONSOLE_PUTCHAR
  ecall
  ret

  # console_getchar
  .align 4
  .globl sbi_console_getchar; 
sbi_console_getchar:
  li a7, MCALL_CONSOLE_GETCHAR
  ecall
  ret

  # empty
  .align 4
  unimp

  # send ipi
  .align 4
  .globl sbi_send_ipi; 
sbi_send_ipi:
  li a7, MCALL_SEND_IPI
  ecall
  ret

  # clear ipi
  .align 4
  .globl sbi_clear_ipi; 
sbi_clear_ipi:
  li a7, MCALL_CLEAR_IPI
  ecall
  ret

  # timebase
  .align 4
  .globl sbi_timebase; 
sbi_timebase:
  li a0, 10000000 # or, you know, we could provide the correct answer
  ret

  # shutdown
  .align 4
  .globl sbi_shutdown; 
sbi_shutdown:
  li a7, MCALL_SHUTDOWN
  ecall

  # set_timer
  .align 4
  .globl sbi_set_timer; 
sbi_set_timer:
  li a7, MCALL_SET_TIMER
  ecall
  ret

  # mask_interrupt
  .align 4
  .globl sbi_mask_interrupt; 
sbi_mask_interrupt:
  tail __sbi_mask_interrupt

  # unmask_interrupt
  .align 4
  .globl sbi_unmask_interrupt; 
sbi_unmask_interrupt:
  tail __sbi_unmask_interrupt

  # remote_sfence_vm
  .align 4
  .globl sbi_remote_sfence_vm; 
sbi_remote_sfence_vm:
  li a7, MCALL_REMOTE_SFENCE_VM
  ecall
  ret

  # remote_sfence_vm_range
  .align 4
  .globl sbi_remote_sfence_vm_range; 
sbi_remote_sfence_vm_range:
  li a7, MCALL_REMOTE_SFENCE_VM
  ecall
  ret

  # remote_fence_i
  .align 4
  .globl sbi_remote_fence_i; 
sbi_remote_fence_i:
  li a7, MCALL_REMOTE_FENCE_I
  ecall
  ret

  # config_string_base
  .align 4
  .globl sbi_config_string_base; 
sbi_config_string_base:
  li a7, MCALL_CONFIG_STRING_BASE
  ecall
  ret

  # config_string_size
  .align 4
  .globl sbi_config_string_size; 
sbi_config_string_size:
  li a7, MCALL_CONFIG_STRING_SIZE
  ecall
  ret

  # end of SBI trampolines

  .globl do_mcall
do_mcall:
  mv a7, a0
  mv a0, a1
  mv a1, a2
  ecall
  ret

  .align RISCV_PGSHIFT
  .globl _sbi_end
_sbi_end:
