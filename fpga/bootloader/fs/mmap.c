#include "vm.h"
#include "atomic.h"
#include "bbl.h"
#include "bits.h"
#include "mtrap.h"
#include <stdint.h>
#include <errno.h>
#include "dev_map.h"

typedef struct {
  uintptr_t addr;
  size_t length;
  file_t* file;
  size_t offset;
  unsigned refcnt;
  int prot;
} vmr_t;

#define MAX_VMR (RISCV_PGSIZE / sizeof(vmr_t))
static spinlock_t vm_lock = SPINLOCK_INIT;
static vmr_t* vmrs;

static uintptr_t first_free_page;
static size_t next_free_page;
static size_t free_pages;

int have_vm = 1; // unless -p flag is given

static uintptr_t __page_alloc()
{
  kassert(next_free_page != free_pages);
  uintptr_t addr = first_free_page + RISCV_PGSIZE * next_free_page++;
  memset((void*)addr, 0, RISCV_PGSIZE);
  return addr;
}

static vmr_t* __vmr_alloc(uintptr_t addr, size_t length, file_t* file,
                          size_t offset, unsigned refcnt, int prot)
{
  if (!vmrs) {
    spinlock_lock(&vm_lock);
      if (!vmrs)
        vmrs = (vmr_t*)__page_alloc();
    spinlock_unlock(&vm_lock);
  }

  for (vmr_t* v = vmrs; v < vmrs + MAX_VMR; v++) {
    if (v->refcnt == 0) {
      if (file)
        file_incref(file);
      v->addr = addr;
      v->length = length;
      v->file = file;
      v->offset = offset;
      v->refcnt = refcnt;
      v->prot = prot;
      return v;
    }
  }
  return NULL;
}

static void __vmr_decref(vmr_t* v, unsigned dec)
{
  if ((v->refcnt -= dec) == 0)
  {
    if (v->file)
      file_decref(v->file);
  }
}

static size_t pte_ppn(pte_t pte)
{
  return pte >> PTE_PPN_SHIFT;
}

static uintptr_t ppn(uintptr_t addr)
{
  return addr >> RISCV_PGSHIFT;
}

static size_t pt_idx(uintptr_t addr, int level)
{
  size_t idx = addr >> (RISCV_PGLEVEL_BITS*level + RISCV_PGSHIFT);
  return idx & ((1 << RISCV_PGLEVEL_BITS) - 1);
}

static pte_t* __walk_create(uintptr_t addr);

static pte_t* __attribute__((noinline)) __continue_walk_create(uintptr_t addr, pte_t* pte)
{
  *pte = ptd_create(ppn(__page_alloc()));
  return __walk_create(addr);
}

static pte_t* __walk_internal(uintptr_t addr, int create)
{
  pte_t* t = root_page_table;
  for (int i = (VA_BITS - RISCV_PGSHIFT) / RISCV_PGLEVEL_BITS - 1; i > 0; i--) {
    size_t idx = pt_idx(addr, i);
    if (unlikely(!(t[idx] & PTE_V)))
      return create ? __continue_walk_create(addr, &t[idx]) : 0;
    t = (pte_t*)(pte_ppn(t[idx]) << RISCV_PGSHIFT);
  }
  return &t[pt_idx(addr, 0)];
}

static pte_t* __walk(uintptr_t addr)
{
  return __walk_internal(addr, 0);
}


static pte_t* __walk_create(uintptr_t addr)
{
  return __walk_internal(addr, 1);
}

static int __va_avail(uintptr_t vaddr)
{
  pte_t* pte = __walk(vaddr);
  return pte == 0 || *pte == 0;
}

static uintptr_t __vm_alloc(size_t npage)
{
  uintptr_t start = current.brk, end = current.mmap_max - npage*RISCV_PGSIZE;
  for (uintptr_t a = start; a <= end; a += RISCV_PGSIZE)
  {
    if (!__va_avail(a))
      continue;
    uintptr_t first = a, last = a + (npage-1) * RISCV_PGSIZE;
    for (a = last; a > first && __va_avail(a); a -= RISCV_PGSIZE)
      ;
    if (a > first)
      continue;
    return a;
  }
  return 0;
}

#ifdef PTE_TYPE
static inline pte_t prot_to_type(int prot, int user)
{
  prot &= PROT_READ|PROT_WRITE|PROT_EXEC;
  if (user) {
    switch (prot) {
      case PROT_NONE: return PTE_TYPE_SR;
      case PROT_READ: return PTE_TYPE_UR_SR;
      case PROT_WRITE: return PTE_TYPE_URW_SRW;
      case PROT_EXEC: return PTE_TYPE_URX_SRX;
      case PROT_READ|PROT_WRITE: return PTE_TYPE_URW_SRW;
      case PROT_READ|PROT_EXEC: return PTE_TYPE_URX_SRX;
      case PROT_WRITE|PROT_EXEC: return PTE_TYPE_URWX_SRWX;
      case PROT_READ|PROT_WRITE|PROT_EXEC: return PTE_TYPE_URWX_SRWX;
    }
  } else {
    switch (prot) {
      case PROT_NONE:
      case PROT_READ: return PTE_TYPE_SR;
      case PROT_WRITE: return PTE_TYPE_SRW;
      case PROT_EXEC: return PTE_TYPE_SRX;
      case PROT_READ|PROT_WRITE: return PTE_TYPE_SRW;
      case PROT_READ|PROT_EXEC: return PTE_TYPE_SRX;
      case PROT_WRITE|PROT_EXEC: return PTE_TYPE_SRWX;
      case PROT_READ|PROT_WRITE|PROT_EXEC: return PTE_TYPE_SRWX;
    }
  }
  __builtin_unreachable();
}

#else 
static inline pte_t prot_to_type(int prot, int user)
{
  pte_t pte = 0;
  if (prot & PROT_READ) pte |= PTE_R;
  if (prot & PROT_WRITE) pte |= PTE_W;
  if (prot & PROT_EXEC) pte |= PTE_X;
  if (pte == 0) pte = PTE_R;
  if (user) pte |= PTE_U;
  return pte;
}

#endif

int __valid_user_range(uintptr_t vaddr, size_t len)
{
  if (vaddr + len < vaddr)
    return 0;
  return vaddr + len <= current.mmap_max;
}

static int __handle_page_fault(uintptr_t vaddr, int prot)
{
  uintptr_t vpn = vaddr >> RISCV_PGSHIFT;
  vaddr = vpn << RISCV_PGSHIFT;

  pte_t* pte = __walk(vaddr);

  if (pte == 0 || *pte == 0 || !__valid_user_range(vaddr, 1))
    return -1;
  else if (!(*pte & PTE_V))
  {
    uintptr_t ppn = vpn + (first_free_paddr / RISCV_PGSIZE);

    vmr_t* v = (vmr_t*)*pte;
    *pte = pte_create(ppn, prot_to_type(PROT_READ|PROT_WRITE, 0));
    flush_tlb();
    if (v->file)
    {
      size_t flen = MIN(RISCV_PGSIZE, v->length - (vaddr - v->addr));
      ssize_t ret = file_pread(v->file, (void*)vaddr, flen, vaddr - v->addr + v->offset);
      kassert(ret > 0);
      if (ret < RISCV_PGSIZE)
        memset((void*)vaddr + ret, 0, RISCV_PGSIZE - ret);
    }
    else
      memset((void*)vaddr, 0, RISCV_PGSIZE);
    __vmr_decref(v, 1);
    *pte = pte_create(ppn, prot_to_type(v->prot, 1));
  }

  pte_t perms = pte_create(0, prot_to_type(prot, 1));
  if ((*pte & perms) != perms)
    return -1;

  flush_tlb();
  return 0;
}

int handle_page_fault(uintptr_t vaddr, int prot)
{
  spinlock_lock(&vm_lock);
    int ret = __handle_page_fault(vaddr, prot);
  spinlock_unlock(&vm_lock);
  return ret;
}

static void __do_munmap(uintptr_t addr, size_t len)
{
  for (uintptr_t a = addr; a < addr + len; a += RISCV_PGSIZE)
  {
    pte_t* pte = __walk(a);
    if (pte == 0 || *pte == 0)
      continue;

    if (!(*pte & PTE_V))
      __vmr_decref((vmr_t*)*pte, 1);

    *pte = 0;
  }
  flush_tlb(); // TODO: shootdown
}

uintptr_t __do_mmap(uintptr_t addr, size_t length, int prot, int flags, file_t* f, off_t offset)
{
  size_t npage = (length-1)/RISCV_PGSIZE+1;
  if (flags & MAP_FIXED)
  {
    if ((addr & (RISCV_PGSIZE-1)) || !__valid_user_range(addr, length))
      return (uintptr_t)-1;
  }
  else if ((addr = __vm_alloc(npage)) == 0)
    return (uintptr_t)-1;

  vmr_t* v = __vmr_alloc(addr, length, f, offset, npage, prot);
  if (!v)
    return (uintptr_t)-1;

  for (uintptr_t a = addr; a < addr + length; a += RISCV_PGSIZE)
  {
    pte_t* pte = __walk_create(a);
    kassert(pte);

    if (*pte)
      __do_munmap(a, RISCV_PGSIZE);

    *pte = (pte_t)v;
  }

  if (!have_vm || (flags & MAP_POPULATE))
    for (uintptr_t a = addr; a < addr + length; a += RISCV_PGSIZE)
      kassert(__handle_page_fault(a, prot) == 0);

  return addr;
}

int do_munmap(uintptr_t addr, size_t length)
{
  if ((addr & (RISCV_PGSIZE-1)) || !__valid_user_range(addr, length))
    return -EINVAL;

  spinlock_lock(&vm_lock);
    __do_munmap(addr, length);
  spinlock_unlock(&vm_lock);

  return 0;
}

uintptr_t do_mmap(uintptr_t addr, size_t length, int prot, int flags, int fd, off_t offset)
{
  if (!(flags & MAP_PRIVATE) || length == 0 || (offset & (RISCV_PGSIZE-1)))
    return -EINVAL;

  file_t* f = NULL;
  if (!(flags & MAP_ANONYMOUS) && (f = file_get(fd)) == NULL)
    return -EBADF;

  spinlock_lock(&vm_lock);
    addr = __do_mmap(addr, length, prot, flags, f, offset);

    if (addr < current.brk_max)
      current.brk_max = addr;
  spinlock_unlock(&vm_lock);

  if (f) file_decref(f);
  return addr;
}

uintptr_t __do_brk(size_t addr)
{
  uintptr_t newbrk = addr;
  if (addr < current.brk_min)
    newbrk = current.brk_min;
  else if (addr > current.brk_max)
    newbrk = current.brk_max;

  if (current.brk == 0)
    current.brk = ROUNDUP(current.brk_min, RISCV_PGSIZE);

  uintptr_t newbrk_page = ROUNDUP(newbrk, RISCV_PGSIZE);
  if (current.brk > newbrk_page)
    __do_munmap(newbrk_page, current.brk - newbrk_page);
  else if (current.brk < newbrk_page)
    kassert(__do_mmap(current.brk, newbrk_page - current.brk, -1, MAP_FIXED|MAP_PRIVATE|MAP_ANONYMOUS, 0, 0) == current.brk);
  current.brk = newbrk_page;

  return newbrk;
}

uintptr_t do_brk(size_t addr)
{
  spinlock_lock(&vm_lock);
    addr = __do_brk(addr);
  spinlock_unlock(&vm_lock);
  
  return addr;
}

uintptr_t do_mremap(uintptr_t addr, size_t old_size, size_t new_size, int flags)
{
  return -ENOSYS;
}


#ifdef PTE_TYPE
uintptr_t do_mprotect(uintptr_t addr, size_t length, int prot)
{
  uintptr_t res = 0;
  if ((addr) & (RISCV_PGSIZE-1))
    return -EINVAL;

  spinlock_lock(&vm_lock);
    for (uintptr_t a = addr; a < addr + length; a += RISCV_PGSIZE)
    {
      pte_t* pte = __walk(a);
      if (pte == 0 || *pte == 0) {
        res = -ENOMEM;
        break;
      }
  
      if (!(*pte & PTE_V)) {
        vmr_t* v = (vmr_t*)*pte;
        if((v->prot ^ prot) & ~v->prot){
          //TODO:look at file to find perms
          res = -EACCES;
          break;
        }
        v->prot = prot;
      } else {
        if (((prot & PROT_WRITE) && !PTE_UW(*pte))
            || ((prot & PROT_EXEC) && !PTE_UX(*pte))) {
          //TODO:look at file to find perms
          res = -EACCES;
          break;
        }
        *pte = pte_create(pte_ppn(*pte), prot_to_type(prot, 1));
      }
    }
  spinlock_unlock(&vm_lock);
 
  return res;
}
#else 
uintptr_t do_mprotect(uintptr_t addr, size_t length, int prot)
{
  uintptr_t res = 0;
  if ((addr) & (RISCV_PGSIZE-1))
    return -EINVAL;

  spinlock_lock(&vm_lock);
    for (uintptr_t a = addr; a < addr + length; a += RISCV_PGSIZE)
    {
      pte_t* pte = __walk(a);
      if (pte == 0 || *pte == 0) {
        res = -ENOMEM;
        break;
      }
  
      if (!(*pte & PTE_V)) {
        vmr_t* v = (vmr_t*)*pte;
        if((v->prot ^ prot) & ~v->prot){
          //TODO:look at file to find perms
          res = -EACCES;
          break;
        }
        v->prot = prot;
      } else {
        if (!(*pte & PTE_U) ||
            ((prot & PROT_READ) && !(*pte & PTE_R)) ||
            ((prot & PROT_WRITE) && !(*pte & PTE_W)) ||
            ((prot & PROT_EXEC) && !(*pte & PTE_X))) {
          //TODO:look at file to find perms
          res = -EACCES;
          break;
        }
        *pte = pte_create(pte_ppn(*pte), prot_to_type(prot, 1));
      }
    }
  spinlock_unlock(&vm_lock);
 
  return res;
}

#endif



void __map_kernel_range(uintptr_t vaddr, uintptr_t paddr, size_t len, int prot)
{
  uintptr_t n = ROUNDUP(len, RISCV_PGSIZE) / RISCV_PGSIZE;
  uintptr_t offset = paddr - vaddr;
  for (uintptr_t a = vaddr, i = 0; i < n; i++, a += RISCV_PGSIZE)
  {
    pte_t* pte = __walk_create(a);
    kassert(pte);
    *pte = pte_create((a + offset) >> RISCV_PGSHIFT, prot_to_type(prot, 0));
  }
}

void populate_mapping(const void* start, size_t size, int prot)
{
  uintptr_t a0 = ROUNDDOWN((uintptr_t)start, RISCV_PGSIZE);
  for (uintptr_t a = a0; a < (uintptr_t)start+size; a += RISCV_PGSIZE)
  {
    if (prot & PROT_WRITE)
      atomic_add((int*)a, 0);
    else
      atomic_read((int*)a);
  }
}
extern uintptr_t sbi_top_paddr();
extern char sbi_base;
#define extract_ppn(pte) (((uintptr_t)(pte))>>PTE_PPN_SHIFT)
#define start_paddr(ppn) (((uintptr_t)(ppn))<<RISCV_PGSHIFT)
#define end_paddr(ppn,level) (start_paddr(ppn) |((1 << (RISCV_PGSHIFT + RISCV_PGLEVEL_BITS * (level))) - 1))
#define is_ptd(pte) ((((uintptr_t)(pte)) & 0xe) == 0)
#define is_valid_pte(pte) (((uintptr_t)(pte)) & PTE_V)
#define start_vaddr(vpn2,vpn1,vpn0) ((uintptr_t)(((vpn2)<<30|(vpn1)<<21|(vpn0)<<12)))
#define end_vaddr(vpn2,vpn1,vpn0,level) (start_vaddr(vpn2,vpn1,vpn0) | ((1 << (RISCV_PGSHIFT + RISCV_PGLEVEL_BITS * (level))) - 1))

void mmap_display(pte_t* root_paddr)
{

size_t i,j,k;

for (i=0;i<(1<<RISCV_PGLEVEL_BITS);i++){
  pte_t pte = root_paddr[i];
  if (is_valid_pte(pte)){
    if(is_ptd(pte)){
      pte_t * mid_base = (pte_t*)start_paddr(extract_ppn(pte));
      for (j=0;j<(1<<RISCV_PGLEVEL_BITS) ; j++){
        pte_t pte = mid_base[j];
        if (is_valid_pte(pte)) {
          if (is_ptd(pte)){
            pte_t* leaf_base = (pte_t *)start_paddr(extract_ppn(pte));
            for (k=0; k <(1<<RISCV_PGLEVEL_BITS) ; k++){
              pte_t pte = leaf_base[k];
              if (is_valid_pte(pte)){
                if (is_ptd(pte)){
                  printm("bad PTE:");
                }
                printm("%p-%p -> %p-%p\n",
                  start_vaddr(i,j,k),
                  end_vaddr(i,j,k,0),
                  start_paddr(extract_ppn(pte)),
                  end_paddr(extract_ppn(pte),0)
                  );
              }
            }
          }
          else {
            printm("%p-%p -> %p-%p\n",
              start_vaddr(i,j,0),
              end_vaddr(i,j,0,1),
              start_paddr(extract_ppn(pte)),
              end_paddr(extract_ppn(pte),1)
              );
          }
        }
      }
    }
    else {
      printm("%p-%p -> %p-%p\n",
        start_vaddr(i,0,0),
        end_vaddr(i,0,0,2),
        start_paddr(extract_ppn(pte)),
        end_paddr(extract_ppn(pte),2)
        );
    }
  }
}

}

void supervisor_mmap_display()
{
size_t i,j,k;
pte_t * root_paddr = (pte_t*)(read_csr(sptbr)<<RISCV_PGSHIFT);

for (i=0;i<(1<<RISCV_PGLEVEL_BITS);i++){
  pte_t pte = root_paddr[i];
  if (is_valid_pte(pte)){
    if(is_ptd(pte)){
      pte_t * mid_base = (pte_t*)start_paddr(extract_ppn(pte));
      for (j=0;j<(1<<RISCV_PGLEVEL_BITS) ; j++){
        pte_t pte = mid_base[j];
        if (is_valid_pte(pte)) {
          if (is_ptd(pte)){
            pte_t* leaf_base = (pte_t *)start_paddr(extract_ppn(pte));
            for (k=0; k <(1<<RISCV_PGLEVEL_BITS) ; k++){
              pte_t pte = leaf_base[k];
              if (is_valid_pte(pte)){
                if (is_ptd(pte)){
                  printk("bad PTE:");
                }
                printk("%p-%p -> %p-%p\n",
                  start_vaddr(i,j,k),
                  end_vaddr(i,j,k,0),
                  start_paddr(extract_ppn(pte)),
                  end_paddr(extract_ppn(pte),0)
                  );
              }
            }
          }
          else {
            printk("%p-%p -> %p-%p\n",
              start_vaddr(i,j,0),
              end_vaddr(i,j,0,1),
              start_paddr(extract_ppn(pte)),
              end_paddr(extract_ppn(pte),1)
              );
          }
        }
      }
    }
    else {
      printk("%p-%p -> %p-%p\n",
        start_vaddr(i,0,0),
        end_vaddr(i,0,0,2),
        start_paddr(extract_ppn(pte)),
        end_paddr(extract_ppn(pte),2)
        );
    }
  }
}

}

int va_trans(uintptr_t vaddr, uintptr_t* ppa)
{
  pte_t * pte = __walk(vaddr);
  if(is_valid_pte(*pte)){
    *ppa = start_paddr(extract_ppn(*pte)) + (vaddr & (RISCV_PGSIZE - 1));
    return 0;
  }
  return -1;
}



void vm_display()
{
#define SPTBR_PPN_FIELD 0x3fffffffff

  
  write_csr(sptbr,(uintptr_t)root_page_table>> RISCV_PGSHIFT);
  printm("sptbr loaded.\n");

  uintptr_t ptb = read_csr(sptbr);
  uintptr_t ppn = ptb & SPTBR_PPN_FIELD;
  uintptr_t root_paddr = ppn << RISCV_PGSHIFT;
  printm("sptbr's ppn is %p, root_page_table is %p\n",root_paddr,(void*)root_page_table);

}

void vm_init()
{
  size_t mem_pages = mem_size >> RISCV_PGSHIFT;
  free_pages = MAX(8, mem_pages >> (RISCV_PGLEVEL_BITS-1));
  first_free_page = first_free_paddr;
  first_free_paddr += free_pages * RISCV_PGSIZE;

  root_page_table = (void*)__page_alloc();
  __map_kernel_range(DRAM_BASE, DRAM_BASE, first_free_paddr - DRAM_BASE, PROT_READ|PROT_WRITE|PROT_EXEC);
  current.mmap_max = current.brk_max =
    MIN(DRAM_BASE, mem_size - (first_free_paddr - DRAM_BASE));

  // printm("mem pages: %p, free_pages: %p, next_free_page: %p\n",mem_pages,free_pages,next_free_page);
  uintptr_t sbi_start = (uintptr_t)&sbi_base;
  uintptr_t sbi_term = (uintptr_t)sbi_top_paddr();
  uintptr_t sbi_vstart = (uintptr_t)((((DEV_MAP__mem__MASK + 1) << RISCV_PGSHIFT) -(sbi_term - sbi_start)));
  printm("sbi paddr: %p-%p, vaddr %p-%p\n",sbi_start,sbi_term,sbi_vstart,(1lu<<40));
  
  __map_kernel_range(sbi_vstart,sbi_start,sbi_term - sbi_start,PROT_READ|PROT_EXEC);
  

  // vm_display();
  // mmap_display(root_page_table);
}

uintptr_t pk_vm_init()
{
  // size_t mem_pages = mem_size >> RISCV_PGSHIFT;
  // free_pages = MAX(8, mem_pages >> (RISCV_PGLEVEL_BITS-1));
  // first_free_page = first_free_paddr;
  // first_free_paddr += free_pages * RISCV_PGSIZE;

  // root_page_table = (void*)__page_alloc();
  // __map_kernel_range(DRAM_BASE, DRAM_BASE, first_free_paddr - DRAM_BASE, PROT_READ|PROT_WRITE|PROT_EXEC);

  // current.mmap_max = current.brk_max =
  //   MIN(DRAM_BASE, mem_size - (first_free_paddr - DRAM_BASE));

  size_t stack_size = RISCV_PGSIZE * 1024;
  size_t stack_bottom = __do_mmap(current.mmap_max - stack_size, stack_size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, 0, 0);
  kassert(stack_bottom != (uintptr_t)-1);
  current.stack_top = stack_bottom + stack_size;

  uintptr_t kernel_stack_top = __page_alloc() + RISCV_PGSIZE;
  // printm("kernel stack top is %p\n", kernel_stack_top);
  return kernel_stack_top;
}
