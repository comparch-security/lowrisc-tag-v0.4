// See LICENSE for license details.

#include "processor.h"
#include "extension.h"
#include "common.h"
#include "config.h"
#include "sim.h"
#include "mmu.h"
#include "htif.h"
#include "disasm.h"
#include <cinttypes>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <assert.h>
#include <limits.h>
#include <stdexcept>
#include <algorithm>

#undef STATE
#define STATE state

processor_t::processor_t(const char* isa, sim_t* sim, uint32_t id, uint32_t tagsz)
  : sim(sim), ext(NULL), disassembler(new disassembler_t),
    id(id), run(false), debug(false), tagsz(tagsz)
{
  i_insn_trace = 0;
  pfc_skip = 0;
  pfc_nc = 0;

  parse_isa_string(isa);

  if (tagsz >= max_xlen) {
    fprintf(stderr, "error: tagsz can't be greater than xlen");
    abort();
  }

  mmu = new mmu_t(sim, this);

  reset(true);

  register_base_instructions();
}

processor_t::~processor_t()
{
#ifdef RISCV_ENABLE_HISTOGRAM
  if (histogram_enabled)
  {
    fprintf(stderr, "PC Histogram size:%zu\n", pc_histogram.size());
    for (auto it : pc_histogram)
      fprintf(stderr, "%0" PRIx64 " %" PRIu64 "\n", it.first, it.second);
  }
#endif
if (insn_trace_enabled){
  fprintf(stderr,"last %zu instructions proc %3d executed:\n",nc_insn_trace,id);
  for (size_t  k = (i_insn_trace );k < (i_insn_trace + nc_insn_trace) ; k ++){
      int i = k % nc_insn_trace;
      insn_t insn = insn_trace[i].insn;
      word_t pc = insn_trace[i].pc;
      uint64_t bits = insn.bits() & ((1ULL << (8 * insn_length(insn.bits()))) - 1);
      fprintf(stderr, "core %3d: 0x%016" PRIx64 " (0x%08" PRIx64 ") %s\n",
          id,pc, bits, disassembler->disassemble(insn).c_str());
  }
}
  delete mmu;
  delete disassembler;
}

static void bad_isa_string(const char* isa)
{
  fprintf(stderr, "error: bad --isa option %s\n", isa);
  abort();
}

void processor_t::parse_isa_string(const char* str)
{
  std::string lowercase, tmp;
  for (const char *r = str; *r; r++)
    lowercase += std::tolower(*r);

  const char* p = lowercase.c_str();
  const char* all_subsets = "imafdc";

  max_xlen = 64;
  isa = word_t(2) << 62;

  if (strncmp(p, "rv32", 4) == 0)
    max_xlen = 32, isa = word_t(1) << 30, p += 4;
  else if (strncmp(p, "rv64", 4) == 0)
    p += 4;
  else if (strncmp(p, "rv", 2) == 0)
    p += 2;

  if (!*p) {
    p = all_subsets;
  } else if (*p == 'g') { // treat "G" as "IMAFD"
    tmp = std::string("imafd") + (p+1);
    p = &tmp[0];
  } else if (*p != 'i') {
    bad_isa_string(str);
  }

  isa_string = "rv" + std::to_string(max_xlen) + p;
  isa |= 1L << ('s' - 'a'); // advertise support for supervisor mode

  while (*p) {
    isa |= 1L << (*p - 'a');

    if (auto next = strchr(all_subsets, *p)) {
      all_subsets = next + 1;
      p++;
    } else if (*p == 'x') {
      const char* ext = p+1, *end = ext;
      while (islower(*end))
        end++;
      register_extension(find_extension(std::string(ext, end - ext).c_str())());
      p = end;
    } else {
      bad_isa_string(str);
    }
  }

  if (supports_extension('D') && !supports_extension('F'))
    bad_isa_string(str);

  // advertise support for supervisor and user modes
  isa |= 1L << ('s' - 'a');
  isa |= 1L << ('u' - 'a');
}

void state_t::reset()
{
  memset(this, 0, sizeof(*this));
  prv = PRV_M;
  pc = reg_t(DEFAULT_RSTVEC);
  mtvec = DEFAULT_MTVEC;
  load_reservation = -1;
}

void processor_t::set_debug(bool value)
{
  debug = value;
  if (ext)
    ext->set_debug(value);
}

void processor_t::set_histogram(bool value)
{
  histogram_enabled = value;
#ifndef RISCV_ENABLE_HISTOGRAM
  if (value) {
    fprintf(stderr, "PC Histogram support has not been properly enabled;");
    fprintf(stderr, " please re-build the riscv-isa-run project using \"configure --enable-histogram\".\n");
  }
#endif
}

void processor_t::set_nc_insn_trace(size_t value)
{
  insn_trace_enabled = value != 0;
  nc_insn_trace = value;
  insn_trace.resize(nc_insn_trace);
}

void processor_t::set_pfc_skip(size_t value)
{
  pfc_skip = value;
  printf("Proc pfc skip set to %llu\n",pfc_skip);
}

void processor_t::set_pfc_nc(size_t value)
{
  pfc_nc = value;
  printf("Proc pfc nc set to %llu\n",pfc_nc);
}

void processor_t::reset(bool value)
{
  if (run == !value)
    return;
  run = !value;

  state.reset();
  set_csr(CSR_MSTATUS, state.mstatus);

  if (ext)
    ext->reset(); // reset the extension
}

void processor_t::raise_interrupt(word_t which)
{
  throw trap_t(((word_t)1 << (max_xlen-1)) | which);
}

static int ctz(word_t val)
{
  int res = 0;
  if (val)
    while ((val & 1) == 0)
      val >>= 1, res++;
  return res;
}

void processor_t::take_interrupt()
{
  word_t pending_interrupts = state.mip & state.mie;

  word_t mie = get_field(state.mstatus, MSTATUS_MIE);
  word_t m_enabled = state.prv < PRV_M || (state.prv == PRV_M && mie);
  word_t enabled_interrupts = pending_interrupts & ~state.mideleg & -m_enabled;

  word_t sie = get_field(state.mstatus, MSTATUS_SIE);
  word_t s_enabled = state.prv < PRV_S || (state.prv == PRV_S && sie);
  enabled_interrupts |= pending_interrupts & state.mideleg & -s_enabled;

  if (enabled_interrupts)
    raise_interrupt(ctz(enabled_interrupts));
}

static bool validate_priv(word_t priv)
{
  return priv == PRV_U || priv == PRV_S || priv == PRV_M;
}

void processor_t::set_privilege(word_t prv)
{
  assert(validate_priv(prv));
  mmu->flush_tlb();
  state.prv = prv;
}

void processor_t::take_trap(trap_t& t, reg_t epc)
{
  if (debug)
    fprintf(stderr, "core %3d: exception %s, epc 0x%016" PRIx64 "\n",
            id, t.name(), epc.data);

  // by default, trap to M-mode, unless delegated to S-mode
  word_t bit = t.cause();
  word_t deleg = state.medeleg;
  if (bit & ((word_t)1 << (max_xlen-1)))
    deleg = state.mideleg, bit &= ~((word_t)1 << (max_xlen-1));
  if (state.prv <= PRV_S && bit < max_xlen && ((deleg >> bit) & 1)) {
    // handle the trap in S-mode
    state.pc = state.stvec;
    state.scause = t.cause();
    state.sepc = epc.data;
    if (t.has_badaddr())
      state.sbadaddr = t.get_badaddr();

    word_t s = state.mstatus;
    s = set_field(s, MSTATUS_SPIE, get_field(s, MSTATUS_UIE << state.prv));
    s = set_field(s, MSTATUS_SPP, state.prv);
    s = set_field(s, MSTATUS_SIE, 0);
    set_csr(CSR_MSTATUS, s);
    set_privilege(PRV_S);
  } else {
    state.pc = state.mtvec;
    state.mcause = t.cause();
    state.mepc = epc.data;
    if (t.has_badaddr())
      state.mbadaddr = t.get_badaddr();

    word_t s = state.mstatus;
    s = set_field(s, MSTATUS_MPIE, get_field(s, MSTATUS_UIE << state.prv));
    s = set_field(s, MSTATUS_MPP, state.prv);
    s = set_field(s, MSTATUS_MIE, 0);
    set_csr(CSR_MSTATUS, s);
    set_privilege(PRV_M);
  }

  yield_load_reservation();
}

void processor_t::disasm(insn_t insn)
{
  uint64_t bits = insn.bits() & ((1ULL << (8 * insn_length(insn.bits()))) - 1);
  fprintf(stderr, "core %3d: 0x%016" PRIx64 " (0x%08" PRIx64 ") %s\n",
          id, state.pc.data, bits, disassembler->disassemble(insn).c_str());
}

static bool validate_vm(int max_xlen, word_t vm)
{
  if (max_xlen == 64 && (vm == VM_SV39 || vm == VM_SV48))
    return true;
  if (max_xlen == 32 && vm == VM_SV32)
    return true;
  return vm == VM_MBARE;
}

void processor_t::set_csr(int which, word_t val)
{
  val = zext_xlen(val);
  word_t delegable_ints = MIP_SSIP | MIP_STIP | (1 << IRQ_COP);
  word_t all_ints = delegable_ints | MIP_MSIP | MIP_MTIP;
  switch (which)
  {
    case CSR_FFLAGS:
      dirty_fp_state;
      state.fflags = val & (FSR_AEXC >> FSR_AEXC_SHIFT);
      break;
    case CSR_FRM:
      dirty_fp_state;
      state.frm = val & (FSR_RD >> FSR_RD_SHIFT);
      break;
    case CSR_FCSR:
      dirty_fp_state;
      state.fflags = (val & FSR_AEXC) >> FSR_AEXC_SHIFT;
      state.frm = (val & FSR_RD) >> FSR_RD_SHIFT;
      break;
    case CSR_MSTATUS: {
      if ((val ^ state.mstatus) &
          (MSTATUS_VM | MSTATUS_MPP | MSTATUS_MPRV | MSTATUS_PUM | MSTATUS_MXR))
        mmu->flush_tlb();

      word_t mask = MSTATUS_SIE | MSTATUS_SPIE | MSTATUS_MIE | MSTATUS_MPIE
                 | MSTATUS_SPP | MSTATUS_FS | MSTATUS_MPRV | MSTATUS_PUM
                 | MSTATUS_MXR | (ext ? MSTATUS_XS : 0);

      if (validate_vm(max_xlen, get_field(val, MSTATUS_VM)))
        mask |= MSTATUS_VM;
      if (validate_priv(get_field(val, MSTATUS_MPP)))
        mask |= MSTATUS_MPP;

      state.mstatus = (state.mstatus & ~mask) | (val & mask);

      bool dirty = (state.mstatus & MSTATUS_FS) == MSTATUS_FS;
      dirty |= (state.mstatus & MSTATUS_XS) == MSTATUS_XS;
      if (max_xlen == 32)
        state.mstatus = set_field(state.mstatus, MSTATUS32_SD, dirty);
      else
        state.mstatus = set_field(state.mstatus, MSTATUS64_SD, dirty);

      // spike supports the notion of xlen < max_xlen, but current priv spec
      // doesn't provide a mechanism to run RV32 software on an RV64 machine
      xlen = max_xlen;
      break;
    }
    case CSR_MIP: {
      word_t mask = MIP_SSIP | MIP_STIP;
      state.mip = (state.mip & ~mask) | (val & mask);
      break;
    }
    case CSR_MIE:
      state.mie = (state.mie & ~all_ints) | (val & all_ints);
      break;
    case CSR_MIDELEG:
      state.mideleg = (state.mideleg & ~delegable_ints) | (val & delegable_ints);
      break;
    case CSR_MEDELEG: {
      word_t mask = 0;
#define DECLARE_CAUSE(name, value) mask |= 1ULL << (value);
#include "encoding.h"
#undef DECLARE_CAUSE
      state.medeleg = (state.medeleg & ~mask) | (val & mask);
      break;
    }
    case CSR_MUCOUNTEREN:
      state.mucounteren = val & 7;
      break;
    case CSR_MSCOUNTEREN:
      state.mscounteren = val & 7;
      break;
    case CSR_SSTATUS: {
      word_t mask = SSTATUS_SIE | SSTATUS_SPIE | SSTATUS_SPP | SSTATUS_FS
                 | SSTATUS_XS | SSTATUS_PUM;
      return set_csr(CSR_MSTATUS, (state.mstatus & ~mask) | (val & mask));
    }
    case CSR_SIP:
      return set_csr(CSR_MIP,
                     (state.mip & ~state.mideleg) | (val & state.mideleg));
    case CSR_SIE:
      return set_csr(CSR_MIE,
                     (state.mie & ~state.mideleg) | (val & state.mideleg));
    case CSR_SEPC: state.sepc = val; break;
    case CSR_STVEC: state.stvec = val >> 2 << 2; break;
    case CSR_SPTBR: state.sptbr = val; break;
    case CSR_SSCRATCH: state.sscratch = val; break;
    case CSR_SCAUSE: state.scause = val; break;
    case CSR_SBADADDR: state.sbadaddr = val; break;
    case CSR_MEPC: state.mepc = val; break;
    case CSR_MTVEC: state.mtvec = val >> 2 << 2; break;
    case CSR_MSCRATCH: state.mscratch = val; break;
    case CSR_MCAUSE: state.mcause = val; break;
    case CSR_MBADADDR: state.mbadaddr = val; break;
    case CSR_MTAGCTRL:
    case CSR_STAGCTRL:
    case CSR_UTAGCTRL: state.tagctrl = val; break;
  }
}

word_t processor_t::get_csr(int which)
{
  switch (which)
  {
    case CSR_FFLAGS:
      require_fp;
      if (!supports_extension('F'))
        break;
      return state.fflags;
    case CSR_FRM:
      require_fp;
      if (!supports_extension('F'))
        break;
      return state.frm;
    case CSR_FCSR:
      require_fp;
      if (!supports_extension('F'))
        break;
      return (state.fflags << FSR_AEXC_SHIFT) | (state.frm << FSR_RD_SHIFT);
    case CSR_TIME:
    case CSR_INSTRET:
    case CSR_CYCLE:
      if ((state.mucounteren >> (which & (xlen-1))) & 1)
        return get_csr(which + (CSR_MCYCLE - CSR_CYCLE));
      break;
    case CSR_STIME:
    case CSR_SINSTRET:
    case CSR_SCYCLE:
      if ((state.mscounteren >> (which & (xlen-1))) & 1)
        return get_csr(which + (CSR_MCYCLE - CSR_SCYCLE));
      break;
    case CSR_MUCOUNTEREN: return state.mucounteren;
    case CSR_MSCOUNTEREN: return state.mscounteren;
    case CSR_MUCYCLE_DELTA: return 0;
    case CSR_MUTIME_DELTA: return 0;
    case CSR_MUINSTRET_DELTA: return 0;
    case CSR_MSCYCLE_DELTA: return 0;
    case CSR_MSTIME_DELTA: return 0;
    case CSR_MSINSTRET_DELTA: return 0;
    case CSR_MUCYCLE_DELTAH: if (xlen > 32) break; else return 0;
    case CSR_MUTIME_DELTAH: if (xlen > 32) break; else return 0;
    case CSR_MUINSTRET_DELTAH: if (xlen > 32) break; else return 0;
    case CSR_MSCYCLE_DELTAH: if (xlen > 32) break; else return 0;
    case CSR_MSTIME_DELTAH: if (xlen > 32) break; else return 0;
    case CSR_MSINSTRET_DELTAH: if (xlen > 32) break; else return 0;
    case CSR_MCYCLE: return state.minstret;
    case CSR_MINSTRET: return state.minstret;
    case CSR_MCYCLEH: if (xlen > 32) break; else return state.minstret >> 32;
    case CSR_MINSTRETH: if (xlen > 32) break; else return state.minstret >> 32;
    case CSR_SSTATUS: {
      word_t mask = SSTATUS_SIE | SSTATUS_SPIE | SSTATUS_SPP | SSTATUS_FS
                 | SSTATUS_XS | SSTATUS_PUM;
      word_t sstatus = state.mstatus & mask;
      if ((sstatus & SSTATUS_FS) == SSTATUS_FS ||
          (sstatus & SSTATUS_XS) == SSTATUS_XS)
        sstatus |= (xlen == 32 ? SSTATUS32_SD : SSTATUS64_SD);
      return sstatus;
    }
    case CSR_SIP: return state.mip & state.mideleg;
    case CSR_SIE: return state.mie & state.mideleg;
    case CSR_SEPC: return state.sepc;
    case CSR_SBADADDR: return state.sbadaddr;
    case CSR_STVEC: return state.stvec;
    case CSR_SCAUSE:
      if (max_xlen > xlen)
        return state.scause | ((state.scause >> (max_xlen-1)) << (xlen-1));
      return state.scause;
    case CSR_SPTBR: return state.sptbr;
    case CSR_SASID: return 0;
    case CSR_SSCRATCH: return state.sscratch;
    case CSR_MSTATUS: return state.mstatus;
    case CSR_MIP: return state.mip;
    case CSR_MIE: return state.mie;
    case CSR_MEPC: return state.mepc;
    case CSR_MSCRATCH: return state.mscratch;
    case CSR_MCAUSE: return state.mcause;
    case CSR_MBADADDR: return state.mbadaddr;
    case CSR_MISA: return isa;
    case CSR_MARCHID: return 0;
    case CSR_MIMPID: return 0;
    case CSR_MVENDORID: return 0;
    case CSR_MHARTID: return id;
    case CSR_MTVEC: return state.mtvec;
    case CSR_MEDELEG: return state.medeleg;
    case CSR_MIDELEG: return state.mideleg;
    case CSR_MTAGCTRL:
    case CSR_STAGCTRL:
    case CSR_UTAGCTRL: return state.tagctrl;
  }
  throw trap_illegal_instruction();
}

reg_t illegal_instruction(processor_t* p, insn_t insn, reg_t pc)
{
  throw trap_illegal_instruction();
}

insn_func_t processor_t::decode_insn(insn_t insn)
{
  // look up opcode in hash table
  size_t idx = insn.bits() % OPCODE_CACHE_SIZE;
  insn_desc_t desc = opcode_cache[idx];

  if (unlikely(insn.bits() != desc.match)) {
    // fall back to linear search
    insn_desc_t* p = &instructions[0];
    while ((insn.bits() & p->mask) != p->match)
      p++;
    desc = *p;

    if (p->mask != 0 && p > &instructions[0]) {
      if (p->match != (p-1)->match && p->match != (p+1)->match) {
        // move to front of opcode list to reduce miss penalty
        while (--p >= &instructions[0])
          *(p+1) = *p;
        instructions[0] = desc;
      }
    }

    opcode_cache[idx] = desc;
    opcode_cache[idx].match = insn.bits();
  }

  return xlen == 64 ? desc.rv64 : desc.rv32;
}

void processor_t::register_insn(insn_desc_t desc)
{
  instructions.push_back(desc);
}

void processor_t::build_opcode_map()
{
  struct cmp {
    bool operator()(const insn_desc_t& lhs, const insn_desc_t& rhs) {
      if (lhs.match == rhs.match)
        return lhs.mask > rhs.mask;
      return lhs.match > rhs.match;
    }
  };
  std::sort(instructions.begin(), instructions.end(), cmp());

  for (size_t i = 0; i < OPCODE_CACHE_SIZE; i++)
    opcode_cache[i] = {1, 0, &illegal_instruction, &illegal_instruction};
}

void processor_t::register_extension(extension_t* x)
{
  for (auto insn : x->get_instructions())
    register_insn(insn);
  build_opcode_map();
  for (auto disasm_insn : x->get_disasms())
    disassembler->add_insn(disasm_insn);
  if (ext != NULL)
    throw std::logic_error("only one extension may be registered");
  ext = x;
  x->set_processor(this);
}

void processor_t::register_base_instructions()
{
  #define DECLARE_INSN(name, match, mask) \
    word_t name##_match = (match), name##_mask = (mask);
  #include "encoding.h"
  #undef DECLARE_INSN

  #define DEFINE_INSN(name) \
    REGISTER_INSN(this, name, name##_match, name##_mask)
  #include "insn_list.h"
  #undef DEFINE_INSN

  register_insn({0, 0, &illegal_instruction, &illegal_instruction});
  build_opcode_map();
}

bool processor_t::load(word_t addr, size_t len, uint8_t* bytes)
{
  return false;
}

bool processor_t::store(word_t addr, size_t len, const uint8_t* bytes)
{
  switch (addr)
  {
    case 0:
      state.mip &= ~MIP_MSIP;
      if (bytes[0] & 1)
        state.mip |= MIP_MSIP;
      return true;

    default:
      return false;
  }
}
