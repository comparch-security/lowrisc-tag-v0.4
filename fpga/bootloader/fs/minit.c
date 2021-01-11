#include "mtrap.h"
#include "atomic.h"
#include "vm.h"
// #include "fp_emulation.h"
#include "uart.h"
#include <string.h>
#include "bits.h"
#include "rtc.h"
#include "ff.h"


pte_t* root_page_table;
uintptr_t first_free_paddr;
uintptr_t mem_size;
uintptr_t num_harts;
volatile uint64_t* mtime;

static void mstatus_init()
{
  // Enable FPU and set VM mode
  uintptr_t ms = 0;
  ms = INSERT_FIELD(ms, MSTATUS_VM, VM_CHOICE);
  ms = INSERT_FIELD(ms, MSTATUS_FS, 1);
  write_csr(mstatus, ms);

  // Make sure the hart actually supports the VM mode we want
  ms = read_csr(mstatus);
  assert(EXTRACT_FIELD(ms, MSTATUS_VM) == VM_CHOICE);

  // Enable user/supervisor use of perf counters
  write_csr(mucounteren, -1);
  write_csr(mscounteren, -1);
  write_csr(mie, ~MIP_MTIP); // disable timer; enable other interrupts
}

// send S-mode interrupts and most exceptions straight to S-mode
static void delegate_traps()
{
  uintptr_t interrupts = MIP_SSIP | MIP_STIP;
  uintptr_t exceptions =
    (1U << CAUSE_MISALIGNED_FETCH) |
    (1U << CAUSE_FAULT_FETCH) |
    (1U << CAUSE_BREAKPOINT) |
    (1U << CAUSE_FAULT_LOAD) |
    (1U << CAUSE_FAULT_STORE) |
    (1U << CAUSE_BREAKPOINT) |
    (1U << CAUSE_USER_ECALL) |
    (1U << CAUSE_TAG_CHECK_FAIL);

  write_csr(mideleg, interrupts);
  write_csr(medeleg, exceptions);
  assert(read_csr(mideleg) == interrupts);
  assert(read_csr(medeleg) == exceptions);
}

// static void fp_init()
// {
//   assert(read_csr(mstatus) & MSTATUS_FS);

// #ifdef __riscv_hard_float
//   if (!supports_extension('D'))
//     die("FPU not found; recompile pk with -msoft-float");
//   for (int i = 0; i < 32; i++)
//     init_fp_reg(i);
//   write_csr(fcsr, 0);
// #else
//   if (supports_extension('D'))
//     die("FPU unexpectedly found; recompile with -mhard-float");
// #endif
// }


hls_t* hls_init(uintptr_t id)
{
  hls_t* hls = OTHER_HLS(id);
  memset(hls, 0, sizeof(*hls));
  return hls;
}

uintptr_t sbi_top_paddr()
{
  extern char _end;
  return ROUNDUP((uintptr_t)&_end, RISCV_PGSIZE);
}

static void memory_init()
{
  mem_size = mem_size / MEGAPAGE_SIZE * MEGAPAGE_SIZE;
  first_free_paddr = sbi_top_paddr() + num_harts * RISCV_PGSIZE;
}

static void hart_init()
{
  mstatus_init();
  // fp_init();
  delegate_traps();
}

// FATFS FatFs; /* Work Area (file system object) for logical drive */
void tagconfig_init()
{
  write_csr(mstagctrlen,  0); //disable supervisor change mtagctrl
  write_csr(mutagctrlen,  0); //disable user change mtagctrl
  long tagcheck = TMASK_JMP_CHECK;
  long tagprop  = TMASK_STORE_PROP | TMASK_LOAD_PROP | TMASK_JMP_PROP;
  write_csr(mtagctrl,  0); //clear mtagctrl
  write_csr(mtagctrl,  tagcheck | tagprop);
}

void init_first_hart()
{
  printm("init_first_hart\n");
  tagconfig_init();
  hart_init();
  hls_init(0); // this might get called again from parse_config_string
  uart_init(); // init early if need to debug config string
  parse_config_string();
  memory_init();
  uart_init();
  // f_mount(&FatFs,"0:",1);
  // f_mount(&FatFs,"0:",1);
  boot_loader();
}

void init_other_hart()
{
  hart_init();

  // wait until hart 0 discovers us
  while (*(uint64_t * volatile *)&HLS()->timecmp == NULL)
    ;

  // boot_other_hart();
}


void enter_supervisor_mode(void (*fn)(uintptr_t), uintptr_t stack)
{
  uintptr_t mepc,sptbr;

  uintptr_t mstatus = read_csr(mstatus);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPP, PRV_S);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPIE, 0);
  write_csr(mstatus, mstatus);
  write_csr(mscratch, MACHINE_STACK_TOP() - MENTRY_FRAME_SIZE);
  write_csr(mepc, fn);
  write_csr(sptbr, (uintptr_t)root_page_table >> RISCV_PGSHIFT);
  // write_csr(sptbr, (uintptr_t)root_page_table);
  
  //enable rtc2 interrupt
  rtc2_update_cmp(5000000);
  set_csr(mstatus,MSTATUS_MIE);

  mstatus = read_csr(mstatus);
  mepc = read_csr(mepc);
  sptbr = read_csr(sptbr) << RISCV_PGSHIFT;
  // printm("mstatus:%p,mepc:%p,sptbr:%p\n",mstatus,mepc,sptbr);
  asm volatile ("mv a0, %0; mv sp, %0; mret" : : "r" (stack));
  // asm volatile ("mret");
  __builtin_unreachable();
}
