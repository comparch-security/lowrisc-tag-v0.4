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

pfc_response pfc0,pfc;

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
typedef union {
  uint64_t buf[MAX_ARGS];
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
  printk("begin running loaded program.\n");
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
  
  printk("before init stack\n");

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

  get_pfc(&pfc0);

  start_uarch_counters();

  trapframe_t tf;
  // init_tf(&tf, 0x3f2bd0 , stack_top);
  init_tf(&tf, current.entry, stack_top);
  printk("current entry is %p\n",current.entry);
  __clear_cache(0, 0);
  write_csr(sscratch, kstack_top);
  printk("before starting user mode.\n");
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

static int args_parser(char * str, arg_buf * args){
  static char buf [256];
  strncpy(buf,str,256);


  int argc = 0;
  char* arg = strtok(buf," ");
  do {
    if (strlen(arg) == 0) 
      continue;
    args->argv[argc++] = arg;
  }while((arg = strtok(NULL," ")) != NULL);

  printk("total number of args: %d\n",argc);
  for(int i = 0; i < argc; i++){
    printk("argv[%d] is %s\n",i,args->argv[i]);
  }

  for (int i = 0; i< argc; i++){
    if (args->argv[i] == NULL)
      continue;

    if (strncmp(args->argv[i],"<",1) == 0) {
      // redirect stdin.
      int ret = file_reopen(0,args->argv[i+1],O_RDONLY);
      if (ret) printk("file reopen failed, fd = %d, fname = %s\n",0,args->argv[i+1]);

      args->argv[i] = args->argv[i+1] = NULL;
    }

    else if (strncmp(args->argv[i],">",1) == 0) {
      // redirect stdout
      int ret = file_reopen(1,args->argv[i+1],O_WRONLY|O_CREAT);
      if (ret ) printk("file reopen failed, fd = %d, fname = %s\n",1,args->argv[i+1]);
    
      args->argv[i] = args->argv[i+1] = NULL;
    }

  }

  //rearrange parameters
  for (int i = 0; i< argc; i++)
  {
    if (args->argv[i] == NULL){
      for (int j = i+1; j< argc; j++){
        if (args->argv[j] != NULL){
          args->argv[i] = args->argv[j];
          args->argv[j] = NULL;
        }
      }
    }
  }

  size_t new_argc = argc;
  for (int i = 0; i< argc;i++)
  {
    if (args->argv[i] == 0){
      new_argc = i;
      break;
    }
  }

  if (argc != new_argc){
    printk("After dealing with stdin/out redirection, total number of args: %d\n",argc);
    for(int i = 0; i < new_argc; i++){
      printk("argv[%d] is %s\n",i,args->argv[i]);
    }
  }
  
  argc = new_argc;

  return argc;
}

static int read_batch(const char * bn,size_t * pargc, arg_buf * args)
{
  if(!bn)
    return -1;
  
  static char buf [1024];

  file_t * batch = file_open(bn,O_RDONLY);
  if(!batch) return -1;

  ssize_t rsize = file_read(batch,buf,1024);
  if(rsize < 0) {
    file_decref(batch);
    file_decref(batch);
    return -1;
  }

  char * command = strtok(buf,"\n");
  // batch has two lines, the first line is a cd command.
  if (strncmp("cd",command,2) == 0){
    char * pnonspace = command + 2; // find the first non-blank char
    while(*pnonspace && *pnonspace == ' ')
      pnonspace++;
    
    if(*pnonspace) file_chdir(pnonspace);
  }

  command = strtok(NULL,"\n");
  if(!command) return -1;

  // the second line of batch, running SPEC benchmark;
  *pargc = args_parser(command,args);

  return 0;  
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

 #if(ELFINSD==1)
  /* We need to mount CF twice */
  long fr = file_mount();
  fr = file_mount();

  // s_mode_ftest();
  
  if(read_batch("0:/run.sh",&argc,&args) == -1){
    // file_chdir("/0:/400.perlbench");
    //  file_chdir("/0:/403.gcc");
    // file_chdir("/0:/429.mcf");
    // file_chdir("/0:/445.gobmk");
    // file_chdir("/0:/456.hmmer");
    // file_chdir("/0:/462.libquantum");
    // file_chdir("/0:/464.h264ref");
    // file_chdir("/0:/471.omnetpp");
    // file_chdir("/0:/473.astar");
    file_chdir("/0:/483.xalancbmk");
    // file_chdir("/0:/401.bzip2");
    // file_chdir("/0:/458.sjeng");
    // file_chdir("/0:/ehtest");
    // file_chdir("/0:/");
    static char argstr [256] = 

     /**** test input ***/

      // "hmmer --fixed 0 --mean 325 --num 45000 --sd 200 --seed 0 bombesin.hmm"
      // "mcf inp.in"
      // "omnetpp omnetpp.ini"
      // "gobmk --quiet --mode gtp < capture.tst"
      // "gobmk --quiet --mode gtp < connect.tst"
      // "gobmk --quiet --mode gtp < connect_rot.tst"
      // "gobmk --quiet --mode gtp < connection.tst"
      // "gobmk --quiet --mode gtp < connection_rot.tst"
      // "gobmk --quiet --mode gtp < cutstone.tst"
      // "gobmk --quiet --mode gtp < dniwog.tst"
      // "gobmk --mode gtp"
      // "astar lake.cfg"
      // "h264ref -d foreman_test_encoder_baseline.cfg"
      // "perlbench -I. -I./lib attrs.pl"
      // "perlbench -I. -I./lib gv.pl"
      // "perlbench  -I. -I./lib makerand.pl"
      // "perlbench  -I. -I./lib pack.pl"
      // "perlbench  -I. -I./lib regmesg.pl"
      // "perlbench  -I. -I./lib test.pl"
      // "payload < test.txt"
      // "payload"
      //  "gcc cccp.i -o cccp.s"
      // "Xalan -v test.xml xalanc.xsl"
      // "libquantum 33 5"
      // "bzip2 input.program 5"
      // "bzip2 dryer.jpg 2"
      // "sjeng test.txt"

    /***** test input end ****/

    /***** ref input ******/
      // "libquantum 1397 8"
      // "mcf inp.in"
      // "sjeng ref.txt"
      "Xalan -v t5.xml xalanc.xsl"
      // "astar BigLakes2048.cfg"
      // "astar rivers.cfg"
      // "omnetpp omnetpp.ini"
      // "h264ref -d foreman_ref_encoder_baseline.cfg"
      // "h264ref -d foreman_ref_encoder_main.cfg"
      // "h264ref -d sss_encoder_main.cfg"
      // "hmmer nph3.hmm swiss41"
      // "hmmer --fixed 0 --mean 500 --num 500000 --sd 350 --seed 0 retro.hmm"
      // "hello"


    /***** ref input end *****/
      // "structra.riscv"
      ;
    argc = args_parser(argstr,&args); 

  }

  load_elf(args.argv[0], &current);
  // supervisor_mmap_display();
#else
  extern char _payload_start, _payload_end;
  static char argstr [256] ="empty"; //" " would cause bug
  argc = args_parser(argstr,&args);
  if(load_elf_from_DRAM(&_payload_start, &_payload_end-&_payload_start, &current))
    printk("load payload from dram successed\n");
#endif

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
