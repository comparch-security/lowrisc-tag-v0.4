// See LICENSE for license details.

#include "mmu.h"
#include "sim.h"
#include "processor.h"

mmu_t::mmu_t(sim_t* sim, processor_t* proc)
 : sim(sim), proc(proc)
{
  flush_tlb();
}

mmu_t::~mmu_t()
{
}

void mmu_t::flush_icache()
{
  for (size_t i = 0; i < ICACHE_ENTRIES; i++)
    icache[i].tag = -1;
}

void mmu_t::flush_tlb()
{
  memset(tlb_insn_tag, -1, sizeof(tlb_insn_tag));
  memset(tlb_load_tag, -1, sizeof(tlb_load_tag));
  memset(tlb_store_tag, -1, sizeof(tlb_store_tag));

  flush_icache();
}

word_t mmu_t::translate(word_t addr, access_type type)
{
  if (!proc)
    return addr;

  word_t mode = proc->state.prv;
  if (type != FETCH) {
    if (get_field(proc->state.mstatus, MSTATUS_MPRV))
      mode = get_field(proc->state.mstatus, MSTATUS_MPP);
  }
  if (get_field(proc->state.mstatus, MSTATUS_VM) == VM_MBARE)
    mode = PRV_M;

  if (mode == PRV_M) {
    word_t msb_mask = (word_t(2) << (proc->xlen-1))-1; // zero-extend from xlen
    return addr & msb_mask;
  }
  return walk(addr, type, mode) | (addr & (PGSIZE-1));
}

const uint16_t* mmu_t::fetch_slow_path(word_t addr)
{
  word_t paddr = translate(addr, FETCH);
  if (sim->addr_is_mem(paddr)) {
    refill_tlb(addr, paddr, FETCH);
    return (const uint16_t*)sim->addr_to_mem(paddr);
  } else {
    if (!sim->mmio_load(paddr, sizeof fetch_temp, (uint8_t*)&fetch_temp))
      throw trap_instruction_access_fault(addr);
    return &fetch_temp;
  }
}

void mmu_t::load_slow_path(word_t addr, word_t len, uint8_t* bytes, uint64_t *tag)
{
  word_t paddr = translate(addr, LOAD);
  if (sim->addr_is_mem(paddr)) {
    //memcpy(&bytes, sim->addr_to_mem(paddr), len);
    char *ppaddr = sim->addr_to_mem(paddr);
    if(bytes != NULL) for(word_t i=0; i<len; i++) *(bytes++) = *(ppaddr++);
    if(tag != NULL) *tag = read_tag(paddr);
    if (sim->cacheable(paddr) && tracer.interested_in_range(paddr, paddr + PGSIZE, LOAD))
      tracer.trace(paddr, len, LOAD);
    else
      refill_tlb(addr, paddr, LOAD);
  } else if (!sim->mmio_load(paddr, len, bytes)) {
    throw trap_load_access_fault(addr);
  }
}

void mmu_t::store_slow_path(word_t addr, word_t len, const uint8_t* bytes, uint64_t *tag)
{
  word_t paddr = translate(addr, STORE);
  if (sim->addr_is_mem(paddr)) {
    if(bytes != NULL) memcpy(sim->addr_to_mem(paddr), bytes, len);
    if (sim->cacheable(paddr) && tracer.interested_in_range(paddr, paddr + PGSIZE, STORE))
      tracer.trace(paddr, len, STORE);
    else
      refill_tlb(addr, paddr, STORE);
    if(tag != NULL) write_tag(paddr, *tag);
  } else if (!sim->mmio_store(paddr, len, bytes)) {
    throw trap_store_access_fault(addr);
  }
}

void mmu_t::refill_tlb(word_t vaddr, word_t paddr, access_type type)
{
  word_t idx = (vaddr >> PGSHIFT) % TLB_ENTRIES;
  word_t expected_tag = vaddr >> PGSHIFT;

  if (tlb_load_tag[idx] != expected_tag) tlb_load_tag[idx] = -1;
  if (tlb_store_tag[idx] != expected_tag) tlb_store_tag[idx] = -1;
  if (tlb_insn_tag[idx] != expected_tag) tlb_insn_tag[idx] = -1;

  if (type == FETCH) tlb_insn_tag[idx] = expected_tag;
  else if (type == STORE) tlb_store_tag[idx] = expected_tag;
  else tlb_load_tag[idx] = expected_tag;

  tlb_data[idx] = sim->addr_to_mem(paddr) - vaddr;
}

word_t mmu_t::walk(word_t addr, access_type type, word_t mode)
{
  int levels, ptidxbits, ptesize;
  switch (get_field(proc->get_state()->mstatus, MSTATUS_VM))
  {
    case VM_SV32: levels = 2; ptidxbits = 10; ptesize = 4; break;
    case VM_SV39: levels = 3; ptidxbits = 9; ptesize = 8; break;
    case VM_SV48: levels = 4; ptidxbits = 9; ptesize = 8; break;
    default: abort();
  }

  bool supervisor = mode == PRV_S;
  bool pum = get_field(proc->state.mstatus, MSTATUS_PUM);
  bool mxr = get_field(proc->state.mstatus, MSTATUS_MXR);

  // verify bits xlen-1:va_bits-1 are all equal
  int va_bits = PGSHIFT + levels * ptidxbits;
  word_t mask = (word_t(1) << (proc->xlen - (va_bits-1))) - 1;
  word_t masked_msbs = (addr >> (va_bits-1)) & mask;
  if (masked_msbs != 0 && masked_msbs != mask)
    return -1;

  word_t base = proc->get_state()->sptbr << PGSHIFT;
  int ptshift = (levels - 1) * ptidxbits;
  for (int i = 0; i < levels; i++, ptshift -= ptidxbits) {
    word_t idx = (addr >> (PGSHIFT + ptshift)) & ((1 << ptidxbits) - 1);

    // check that physical address of PTE is legal
    word_t pte_addr = base + idx * ptesize;
    if (!sim->addr_is_mem(pte_addr))
      break;

    void* ppte = sim->addr_to_mem(pte_addr);
    word_t pte = ptesize == 4 ? *(uint32_t*)ppte : *(uint64_t*)ppte;
    word_t ppn = pte >> PTE_PPN_SHIFT;

    if (PTE_TABLE(pte)) { // next level of page table
      base = ppn << PGSHIFT;
    } else if ((pte & PTE_U) ? supervisor && pum : !supervisor) {
      break;
    } else if (!(pte & PTE_R) && (pte & PTE_W)) { // reserved
      break;
    } else if (type == FETCH ? !(pte & PTE_X) :
               type == LOAD ?  !(pte & PTE_R) && !(mxr && (pte & PTE_X)) :
                               !((pte & PTE_R) && (pte & PTE_W))) {
      break;
    } else {
      // set accessed and possibly dirty bits.
      *(uint32_t*)ppte |= PTE_A | ((type == STORE) * PTE_D);
      // for superpage mappings, make a fake leaf PTE for the TLB's benefit.
      word_t vpn = addr >> PGSHIFT;
      return (ppn | (vpn & ((word_t(1) << ptshift) - 1))) << PGSHIFT;
    }
  }

  return -1;
}

void mmu_t::register_memtracer(memtracer_t* t)
{
  flush_tlb();
  tracer.hook(t);
}
