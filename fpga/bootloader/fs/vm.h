#ifndef _VM_H
#define _VM_H

#include "encoding.h"
#include "file.h"
#include <stdint.h>

#define MEGAPAGE_SIZE ((uintptr_t)(RISCV_PGSIZE << RISCV_PGLEVEL_BITS))
#ifdef __riscv64
# define VM_CHOICE VM_SV39
# define VA_BITS 39
# define GIGAPAGE_SIZE (MEGAPAGE_SIZE << RISCV_PGLEVEL_BITS)
#else
# define VM_CHOICE VM_SV32
# define VA_BITS 32
#endif

#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4

#define MAP_PRIVATE 0x2
#define MAP_FIXED 0x10
#define MAP_ANONYMOUS 0x20
#define MAP_POPULATE 0x8000
#define MREMAP_FIXED 0x2

#define supervisor_paddr_valid(start, length) \
  ((uintptr_t)(start) >= current.first_user_vaddr + current.bias \
   && (uintptr_t)(start) + (length) < mem_size \
   && (uintptr_t)(start) + (length) >= (uintptr_t)(start))

// void vm_init();
// void supervisor_vm_init();
uintptr_t pk_vm_init();
int handle_page_fault(uintptr_t vaddr, int prot);
void populate_mapping(const void* start, size_t size, int prot);
void __map_kernel_range(uintptr_t va, uintptr_t pa, size_t len, int prot);
int __valid_user_range(uintptr_t vaddr, size_t len);
uintptr_t __do_mmap(uintptr_t addr, size_t length, int prot, int flags, file_t* file, off_t offset);
uintptr_t do_mmap(uintptr_t addr, size_t length, int prot, int flags, int fd, off_t offset);
int do_munmap(uintptr_t addr, size_t length);
uintptr_t do_mremap(uintptr_t addr, size_t old_size, size_t new_size, int flags);
uintptr_t do_mprotect(uintptr_t addr, size_t length, int prot);
uintptr_t do_brk(uintptr_t addr);

int va_trans(uintptr_t vaddr, uintptr_t* ppa);


typedef uintptr_t pte_t;
extern pte_t* root_page_table;

static inline void flush_tlb()
{
  asm volatile("sfence.vm");
}

static inline pte_t pte_create(uintptr_t ppn, int type)
{
  return (ppn << PTE_PPN_SHIFT) | PTE_V | type;
}


static inline pte_t ptd_create(uintptr_t ppn)
{
  return pte_create(ppn, 0);
}

#define va2pa(va) ({ uintptr_t __va = (uintptr_t)(va); \
  uintptr_t __pa;int __ret = va_trans(__va,&__pa); \
  __ret ? (uintptr_t) 0 : __pa;})

#endif
