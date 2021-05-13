#include "bbl.h"
#include "bits.h"
#include <string.h>
#include <stdbool.h>
#include "uart.h"
#include "file.h"
#include <fcntl.h>
#include "ff.h"
#include "syscall.h"
#include "vm.h"
#include "mtrap.h"
#include "sbi.h"
#include "elf.h"
#include "pfc.h"

elf_info current;
#define NUM_COUNTERS 18
static int uarch_counters_enabled;
static long uarch_counters[NUM_COUNTERS];
static char* uarch_counter_names[NUM_COUNTERS];

static void read_uarch_counters(bool dump)
{
  if (!uarch_counters_enabled)
    return;

  size_t i = 0;
  #define READ_CTR(name) do { \
    while (i >= NUM_COUNTERS) ; \
    long csr = read_csr(name); \
    if (dump && csr) printk("%s = %ld\n", #name, csr - uarch_counters[i]); \
    uarch_counters[i++] = csr; \
  } while (0)
  READ_CTR(0xcc0); READ_CTR(0xcc1); READ_CTR(0xcc2);
  READ_CTR(0xcc3); READ_CTR(0xcc4); READ_CTR(0xcc5);
  READ_CTR(0xcc6); READ_CTR(0xcc7); READ_CTR(0xcc8);
  READ_CTR(0xcc9); READ_CTR(0xcca); READ_CTR(0xccb);
  READ_CTR(0xccc); READ_CTR(0xccd); READ_CTR(0xcce);
  READ_CTR(0xccf); READ_CTR(cycle); READ_CTR(instret);
  #undef READ_CTR
}

static void start_uarch_counters()
{
  read_uarch_counters(false);
}

void dump_uarch_counters()
{
  read_uarch_counters(true);
}

#define MAX_ARGS 64
typedef struct {
  char* argv[MAX_ARGS];
} arg_buf;

static void init_tf(trapframe_t* tf, long pc, long sp)
{
  memset(tf, 0, sizeof(*tf));
  tf->status = (read_csr(sstatus) &~ SSTATUS_SPP &~ SSTATUS_SIE) | SSTATUS_SPIE;
  tf->gpr[2] = sp;
  tf->epc = pc;
}

static void run_loaded_program(size_t argc, char** argv, uintptr_t kstack_top)
{
  //printk("begin running loaded program.\n");
  // copy phdrs to user stack
  size_t stack_top = current.stack_top - current.phdr_size;
  memcpy((void*)stack_top, (void*)current.phdr, current.phdr_size);
  current.phdr = stack_top;

  // copy argv to user stack
  for (size_t i = 0; i < argc; i++) {
    size_t len = strlen((char*)(uintptr_t)argv[i])+1;
    stack_top -= len;
    memcpy((void*)stack_top, (void*)(uintptr_t)argv[i], len);
    argv[i] = (void*)stack_top;
  }
  stack_top &= -sizeof(void*);
  
  printk("-------------------------------\n");

  struct {
    long key;
    long value;
  } aux[] = {
    {AT_ENTRY, current.entry},
    {AT_PHNUM, current.phnum},
    {AT_PHENT, current.phent},
    {AT_PHDR, current.phdr},
    {AT_PAGESZ, RISCV_PGSIZE},
    {AT_SECURE, 0},
    {AT_RANDOM, stack_top},
    {AT_NULL, 0}
  };

  // place argc, argv, envp, auxp on stack
  #define PUSH_ARG(type, value) do { \
    *((type*)sp) = (type)value; \
    sp += sizeof(type); \
  } while (0)

  #define STACK_INIT(type) do { \
    unsigned naux = sizeof(aux)/sizeof(aux[0]); \
    stack_top -= (1 + argc + 1 + 1 + 2*naux) * sizeof(type); \
    stack_top &= -16; \
    long sp = stack_top; \
    PUSH_ARG(type, argc); \
    for (unsigned i = 0; i < argc; i++) \
      PUSH_ARG(type, argv[i]); \
    PUSH_ARG(type, 0); /* argv[argc] = NULL */ \
    PUSH_ARG(type, 0); /* envp[0] = NULL */ \
    for (unsigned i = 0; i < naux; i++) { \
      PUSH_ARG(type, aux[i].key); \
      PUSH_ARG(type, aux[i].value); \
    } \
  } while (0)

  STACK_INIT(uintptr_t);

  if (current.t0) // start timer if so requested
    current.t0 = rdcycle();

  if (current.instret0) // count instret if so requested
    current.instret0 = rdinstret();

  start_uarch_counters();

  trapframe_t tf;
  // init_tf(&tf, 0x3f2bd0 , stack_top);
  init_tf(&tf, current.entry, stack_top);
  //printk("current entry is %p\n",current.entry);
  __clear_cache(0, 0);
  write_csr(sscratch, kstack_top);
  //printk("before starting user mode.\n");
  pfc_log(0);
  start_user(&tf);
}

extern int sys_open(const char* name, int flags, int mode);
extern ssize_t sys_read(int fd, char* buf,size_t n);
extern ssize_t sys_write(int fd,const char* buf, size_t n);


void s_mode_ftest(void)
{

  // uart_send_string("loading elf.\n");
  static char read_buf [128];
  static char comp_buf [128];
  static char cwd_buf [255];

  file_getcwd(cwd_buf,255);
  printk("cwd is %s\n",cwd_buf);

  file_chdir("0:/473.astar");
  file_getcwd(cwd_buf,255);
  printk("cwd is now %s\n",cwd_buf);

  int fd;
  fd = sys_open("astar",O_RDONLY,0);
  if (fd < 0) panic("astar/astar open failed!");
  if (fd_close(fd) == -1) panic("astar/astar close failed !");

  fd = sys_open("../test.txt",O_RDONLY,0);
  if (fd < 0) panic("test.text open failed!");

  ssize_t frd = sys_read(fd,read_buf,90);
  if(frd < 0) panic("test.text read failed!");

  if((fd_close(fd)) == -1) panic("test.text close failed!");
  
  frd = sys_write(1,read_buf,90);

  fd = sys_open("newtest.txt",O_WRONLY|O_CREAT,0);
  if (fd < 0) panic("newtest.txt creation failed.");

  frd = sys_write(fd,read_buf,90);

  if((fd = fd_close(fd) == -1)) panic ("newtest.txt close failed!");

  fd = sys_open("newtest.txt",O_RDONLY,0);
  if (fd < 0) panic("newtest.text reopen failed!");

  frd = sys_read(fd,comp_buf,90);
  if(frd < 0) panic ("newtest.text read failed!");

  frd = sys_write(2,comp_buf,90);

  if((fd = fd_close(fd)) == -1) panic("newtest.txt reclose failed!");


  // file_t * fp = file_open("test.txt",O_RDONLY);
  // if(!fp) printk("test.text open failed!\n");
  
  // ssize_t fr = file_read(fp,read_buf,90);
  // if(fr <= 0) printk("test.text read failed!\n");

  // printk(read_buf);

  printk("\nS mode file R/W test fin.\n");
 
}

char *case_buffer = (char *)(DEV_MAP__io_ext_bram__BASE + DEV_MAP__io_ext_bram__MASK + 1 - 128);
char *spec_case;
char *case_dir;

static int args_parser(arg_buf * args){
  int argc = 0;

  //printk("the original cfg string %s\n",case_buffer);
  
  spec_case = strtok(case_buffer," \n\r");
  case_dir = strtok(NULL," \n\r");
  file_chdir(case_dir);

  do{
    args->argv[argc++] = strtok(NULL," \n\r");
  } while(args->argv[argc-1] && strlen(args->argv[argc-1]) > 0);

  argc--;

  printk("%d arguments in total: ", argc);
  for(int i = 0; i < argc; i++){
    printk("%s ", args->argv[i]);
  }
  printk("\n");

  for (int i = 0; i< argc; i++){
    if (args->argv[i] == NULL)
      continue;

    if (strncmp(args->argv[i],"<",1) == 0) {
      // redirect stdin.
      char dir[256];
      printk("current directory: %s\n", file_getcwd(dir, 256));
      int ret = file_reopen(0,args->argv[i+1],O_RDONLY);
      if (ret) printk("file reopen failed, fd = %d, fname = %s\n",0,args->argv[i+1]);

      args->argv[i] = NULL;
      args->argv[i+1] = NULL;
    }
    else if (strncmp(args->argv[i],">",1) == 0) {
      // redirect stdout
      int ret = file_reopen(1,args->argv[i+1],O_WRONLY|O_CREAT);
      if (ret ) printk("file reopen failed, fd = %d, fname = %s\n",1,args->argv[i+1]);
    
      args->argv[i] = NULL;
      args->argv[i+1] = NULL;
    }

  }

  //rearrange parameters
  int new_argc = 0;
  for (int i = 0; i<argc; i++)
  {
    if(args->argv[i] != NULL)
      args->argv[new_argc++] = args->argv[i];
  }

  if(new_argc != argc) {
    argc = new_argc;
    //printk("%d arguments in total: ", argc);
    //for(int i = 0; i < argc; i++){
    //  printk("%s ", args->argv[i]);
    //}
    //printk("\n");
  }

  return argc;
}

extern void supervisor_mmap_display();

static void rest_of_boot_loader(uintptr_t kstack_top)
{
  long phdrs[128];
  current.phdr = (uintptr_t)phdrs;
  current.phdr_size = sizeof(phdrs);
  current.t0 = (size_t) -1; // enable time counter.
  current.instret0 = (size_t) -1; // enable instret counter.
  arg_buf args;
  size_t argc ;

  /* We need to mount CF twice */
  long fr = file_mount();
  fr = file_mount();
  argc = args_parser(&args);
  load_elf(args.argv[0], &current);
  // supervisor_mmap_display();

  run_loaded_program(argc,args.argv, kstack_top);
}

extern void vm_init();

void boot_loader (void) {
  
  extern char trap_entry;
  write_csr(stvec,&trap_entry);
  write_csr(sscratch,0);
  write_csr(sie,0);

  vm_init();
  file_init();

  enter_supervisor_mode(rest_of_boot_loader,pk_vm_init());    

  while(1);
}
