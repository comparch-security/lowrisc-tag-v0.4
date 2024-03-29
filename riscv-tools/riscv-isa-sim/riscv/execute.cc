// See LICENSE for license details.

#include "processor.h"
#include "mmu.h"
#include <cassert>

static void commit_log_stash_privilege(state_t* state)
{
#ifdef RISCV_ENABLE_COMMITLOG
  state->last_inst_priv = state->prv;
#endif
}

static void commit_log_print_insn(state_t* state, reg_t pc, insn_t insn)
{
#ifdef RISCV_ENABLE_COMMITLOG
  int32_t priv = state->last_inst_priv;
  uint64_t mask = (insn.length() == 8 ? uint64_t(0) : (uint64_t(1) << (insn.length() * 8))) - 1;
  if (state->log_reg_write.addr) {
    fprintf(stderr, "%1d 0x%016" PRIx64 " (0x%08" PRIx64 ") %c%2" PRIu64 " 0x%016" PRIx64 "     DASM(0x%08" PRIx64 ") \n",
            priv,
            pc.data,
            insn.bits() & mask,
            state->log_reg_write.addr & 1 ? 'f' : 'x',
            state->log_reg_write.addr >> 1,
            state->log_reg_write.data,
            insn.bits() & mask);
  } else {
    fprintf(stderr, "%1d 0x%016" PRIx64 " (0x%08" PRIx64 ")                            DASM(0x%08" PRIx64 ") \n", priv, pc.data, insn.bits() & mask, insn.bits() & mask);
  }
  state->log_reg_write.addr = 0;
#endif
}

inline void processor_t::update_histogram(reg_t pc)
{
#ifdef RISCV_ENABLE_HISTOGRAM
  pc_histogram[pc.data]++;
#endif
}

inline void processor_t::update_insn_trace(reg_t pc,insn_t insn){
  if (insn_trace_enabled){
    if(pc.data < 0x80000000){
      insn_trace[i_insn_trace].pc = pc.data;
      insn_trace[i_insn_trace].insn = insn;
      i_insn_trace = (i_insn_trace + 1)%nc_insn_trace;
    }
  }
}

static reg_t execute_insn(processor_t* p, reg_t pc, insn_fetch_t fetch)
{
  commit_log_stash_privilege(p->get_state());
  reg_t npc = fetch.func(p, fetch.insn, pc);
  if (!invalid_pc(npc)) {
    p->update_insn_trace(pc,fetch.insn);
    commit_log_print_insn(p->get_state(), pc, fetch.insn);
    p->update_histogram(pc);
  }
  return npc;
}

// fetch/decode/execute loop
void processor_t::step(size_t n)
{
  while (run && n > 0) {
    size_t instret = 0;
    reg_t pc = state.pc;
    mmu_t* _mmu = mmu;
    bool halt = false;

    #define advance_pc() \
     if (unlikely(invalid_pc(pc))) { \
       switch (pc.data) { \
         case PC_SERIALIZE_BEFORE: state.serialized = true; break; \
         case PC_SERIALIZE_AFTER: instret++; break; \
         default: abort(); \
       } \
       pc = state.pc; \
       break; \
     } else { \
       state.pc = pc; \
       instret++; \
     }

    try
    {
      take_interrupt();

      if (unlikely(debug))
      {
        while (instret < n)
        {
          if (state.minstret + instret == pfc_skip)
            _mmu->clear_stats();
          // if(pfc_nc) printf("pfc_nc: %d\n",pfc_nc);
          if (pfc_nc && state.minstret + instret >= pfc_skip + pfc_nc){
            halt = true;
            break;
          }
          insn_fetch_t fetch = mmu->load_insn(pc.data);
          if (!state.serialized)
            disasm(fetch.insn);
          pc = execute_insn(this, pc, fetch);
          advance_pc();
        }
      }
      else while (instret < n)
      {
        if (state.minstret + instret == pfc_skip)
          _mmu->clear_stats();
        
        // if(pfc_nc) printf("pfc_nc: %d\n",pfc_nc);
        if (pfc_nc && state.minstret + instret >= pfc_skip + pfc_nc){
          halt = true;
          break;
        }
        size_t idx = _mmu->icache_index(pc.data);
        auto ic_entry = _mmu->access_icache(pc.data);

        #define ICACHE_ACCESS(i) { \
          insn_fetch_t fetch = ic_entry->data; \
          ic_entry++; \
          pc = execute_insn(this, pc, fetch); \
          if (i == mmu_t::ICACHE_ENTRIES-1) break; \
          if (unlikely(ic_entry->tag != pc.data)) goto miss; \
          if (unlikely(instret+1 == n)) break; \
          instret++; \
          state.pc = pc; \
        }

        switch (idx) {
          #include "icache.h"
        }

        advance_pc();
        continue;

miss:
        advance_pc();
        // refill I$ if it looks like there wasn't a taken branch
        if (pc.data > (ic_entry-1)->tag && pc.data <= (ic_entry-1)->tag + MAX_INSN_LENGTH)
          _mmu->refill_icache(pc.data, ic_entry);
      }
    }
    catch(trap_t& t)
    {
      take_trap(t, pc);
      n = instret;
    }

    state.minstret += instret;
    n -= instret;

    if(halt){
      run = false;
      printf("Proc halted.\n");
      printf("cycle = %llu\ninstret = %llu\n",this->get_csr(CSR_MCYCLE),state.minstret);
      break;
    }
  }
}
