// See LICENSE for license details.

#include "syscall.h"
#include "bbl.h"
#include "file.h"
#include "vm.h"
#include <string.h>
#include <errno.h>
// #include "uart.h"
#include "sbi.h"
#include "pfc.h"
#include <stdarg.h>
#include <fcntl.h>

#define M_STR(a) #a
#define M_STR2(a) M_STR(a)

typedef long (*syscall_t)(long, long, long, long, long, long, long);

#define CLOCK_FREQ 1000000000



extern pfc_response pfc0,pfc;
extern int vsnprintf(char* out, size_t n, const char* s, va_list vl);

static int vfprintk(file_t* fhdl, const char* s, va_list vl)
{
  char out[256];
  int res = vsnprintf(out,sizeof(out),s,vl);
  file_write(fhdl,out,res < sizeof(out) ? res : sizeof(out));
  return res;
}

static int fprintk(file_t* fhdl, const char* s, ...)
{
  va_list vl;
  va_start(vl,s);

  int ret = vfprintk(fhdl,s,vl);

  va_end(vl);
  return ret;
}


void pfc_log(pfc_response * ppfc, char * fname, int code) 
{

  //file_chdir("/0:/output");
  //file_t * fhdl = file_open(fname,O_WRONLY|O_TRUNC);
  //if(!fhdl) return;

  uint64_t * pfcresp = ppfc->resp;
  uint64_t instret = ppfc->instret;
  uint64_t cycles = ppfc->cycles;

  printk("exit code: %d\n",code);
  printk("total time: %15lld\n",cycles);

#ifdef ENA_PFC

  printk("instret: %15lld\n\n",instret);

  printk("L1I_read,     L1I_readmiss\n");
  printk("%lld  %10lld\n\n", pfcresp[0], pfcresp[1]);
  printk("L1D_read,     L1D_readmiss,     L1D_write,     L1D_writemiss,     L1D_writeback\n");
  printk("%lld  %10lld  %10lld  %10lld  %10lld\n\n", pfcresp[2],pfcresp[3],pfcresp[4],pfcresp[5],pfcresp[6]);

#if(L2Banks!=0)
  printk("L2_read,      L2_readmiss,      L2_write,      L2_writeback\n");
  printk("%lld  %10lld  %10lld  %10lld\n\n", pfcresp[7],pfcresp[8],pfcresp[9],pfcresp[10]);
#endif

#if(ADD_TC)
  printk("TC_readTT,   TC_readTTmiss,    TC_writeTT,   TC_writeTTmiss,    TC_writeTTback\n");
  printk("%lld  %10lld  %10lld  %10lld  %10lld\n\n", pfcresp[11],pfcresp[12],pfcresp[13],pfcresp[14],pfcresp[15]);
  printk("TC_readTM0,  TC_readTM0miss,   TC_writeTM0,  TC_writeTM0miss,   TC_writeTM0back\n");
  printk("%lld  %10lld  %10lld  %10lld  %10lld\n\n", pfcresp[16],pfcresp[17],pfcresp[18],pfcresp[19],pfcresp[20]);
  printk("TC_readTM1,  TC_readTM1miss,   TC_writeTM1,  TC_writeTM1miss,   TC_writeTM1back\n");
  printk("%lld  %10lld  %10lld  %10lld  %10lld\n\n", pfcresp[21],pfcresp[22],pfcresp[23],pfcresp[24],pfcresp[25]);
#endif

#endif

}

void sys_exit(int code)
{
  size_t cycle = rdcycle(),instret=rdinstret();
  get_pfc(&pfc);
  static int already_exited = 0;
  printk("program exited.\n");
  
  if(!already_exited) 
    already_exited = 1;
  else die(code);
  
  if (current.t0)
    printk("%ld cycles\n", cycle - current.t0);
  // if (current.instret0)
  //   printk("%ld instrets\n", instret - current.instret0);
  dump_uarch_counters();
  if(code) {
    char str[] = "error! exit(0xFFFFFFFFFFFFFFFF)\n";
    int i;
    for (i = 0; i < 16; i++) {
      str[29-i] = (code & 0xF) + ((code & 0xF) < 10 ? '0' : 'a'-10);
      code >>= 4;
    }
    sbi_send_string(str);
  }    

  pfc_diff(&pfc, &pfc0);
  pfc_display(&pfc);
  pfc_log(&pfc,"ref" M_STR2(ELFINPNUM) ".log",code);
  die(code);
}

ssize_t sys_read(int fd, char* buf, size_t n)
{
  // printk("sys read at fd %d into vaddr %p\n",fd, buf);
  // printk("#syscall %s, fd= %d, buf= %p, n= %lu\n","read", fd, buf, n);
  ssize_t r = -EBADF;
  file_t* f = file_get(fd);

  if (f)
  {
    r = file_read(f, buf, n);
    file_decref(f);
    // printk("read returns %ld\n",r);
    // printk("read content: %s\n",buf);
  }
  
  return r;
}

ssize_t sys_pread(int fd, char* buf, size_t n, off_t offset)
{
  // printk("sys pread at fd %d from offset %lu \n",fd,offset);
  // printk("#syscall %s, fd= %d, buf= %p, n= %lu, offset= %lu\n","pread", fd, buf, n, offset);
  ssize_t r = -EBADF;
  file_t* f = file_get(fd);

  if (f)
  {
    r = file_pread(f, buf, n, offset);
    file_decref(f);
    // printk("read content: %s\n",buf);
  }

  return r;
}

ssize_t sys_write(int fd, const char* buf, size_t n)
{
  // printk("sys write at fd %d from vaddr %p of length %ld\n",fd, buf,n);
  // printk("data write: %s\n",buf);
    // printk("#syscall %s, fd= %d, buf= %p, n= %d\n","write", fd, buf, n);
  ssize_t r = -EBADF;
  
  if (fd == 1 || fd == 2)
  {
    //stdout and stderr
    sbi_send_buf(buf,n);
    r = n;
  }

  file_t* f = file_get(fd);

  if (f)
  {
    r = file_write(f, buf, n);
    file_decref(f);
  }

  // printk("write returns %ld\n",r);

  return r;
}

static int at_kfd(int dirfd)
{
  if (dirfd == AT_FDCWD)
    return AT_FDCWD;
  file_t* dir = file_get(dirfd);
  if (dir == NULL)
    return -1;
  return (dir - files);
}

int sys_openat(int dirfd, const char* name, int flags, int mode)
{
  // printk("sys open file at %s.\n",name);
  // printk("#syscall %s, dirfd= %d, name= %s, flag= %#x, mode= %d\n","openat", dirfd, name, flags, mode);
  int kfd = at_kfd(dirfd);
  if (kfd != -1) {
    file_t* file = file_openat(kfd, name, flags);
    if (!file)
      return -EBADF;
    extern file_t files [];
    // printk("fd is %d\n",(file- files));
    return (file - files);
  }
  return -EBADF;
}

int sys_open(const char* name, int flags, int mode)
{
  return sys_openat(AT_FDCWD, name, flags, mode);
}

int sys_close(int fd)
{
  // printk("#syscall %s, fd= %d\n","close", fd);
  int ret = fd_close(fd);
  // file_display();
  if (ret < 0)
    return -EBADF;
  return ret;
}

int sys_renameat(int old_fd, const char *old_path, int new_fd, const char *new_path) {
  // int old_kfd = at_kfd(old_fd);
  // int new_kfd = at_kfd(new_fd);
  // if(old_kfd != -1 && new_kfd != -1) {
  //   size_t old_size = strlen(old_path)+1;
  //   size_t new_size = strlen(new_path)+1;
  //   return frontend_syscall(SYS_renameat, old_kfd, va2pa(old_path), old_size,
  //                                          new_kfd, va2pa(new_path), new_size, 0);
  // }
  return -EBADF;
}

int sys_fstat(int fd, void* st)
{
  // printk("#syscall %s, fd= %d, st= %p\n","fstat", fd, st);
  int r = -EBADF;
  file_t* f = file_get(fd);

  if (f)
  {
    r = file_stat(f, (struct stat*)st);
    file_decref(f);
  }
  else if (fd >= 0 && fd <= 2) 
  {
    
    if (fd == 0) 
    {
      ((struct stat*)st)->st_mode = S_IFCHR |S_IRWXG | S_IRWXO | S_IRWXU;
    }
    else 
    {
      ((struct stat*)st)->st_mode = S_IFREG |S_IRWXG | S_IRWXO | S_IRWXU;
    }

    ((struct stat*)st)->st_dev = 0;
    ((struct stat*)st)->st_rdev = 0;
    ((struct stat*)st)->st_ino = 0;
    ((struct stat*)st)->st_nlink = 1;
    ((struct stat*)st)->st_uid = 0;
    ((struct stat*)st)->st_gid = 0;
    ((struct stat*)st)->st_blocks = 0;
    ((struct stat*)st)->st_blksize = 0;
    ((struct stat*)st)->st_size = 0;
    ((struct stat*)st)->st_atime = 0;
    ((struct stat*)st)->st_ctime = 0;
    

    r = 0;
  }
    

  // printk("#syscall fstat returns %d\n",r);
  // file_display();

  return r;
}

int sys_fcntl(int fd, int cmd, int arg)
{
  // printk("#syscall %s, fd= %d, cmd= %d, arg= %d\n","fcntl", fd, cmd, arg);
  int r = -EBADF;
  
  r = file_fcntl(fd,cmd,arg);
  // file_display();

  return r;
}

int sys_ftruncate(int fd, off_t len)
{
  int r = -EBADF;
  // file_t* f = file_get(fd);

  // if (f)
  // {
  //   /* file_truncate not implemented! */
  //   r = file_truncate(f, len);
  //   file_decref(f);
  // }

  return r;
}

int sys_dup(int fd)
{
  int r = -EBADF;
  // file_t* f = file_get(fd);

  // if (f)
  // {
  //   r = file_dup(f);
  //   file_decref(f);
  // }

  return r;
}

ssize_t sys_lseek(int fd, size_t ptr, int dir)
{
  // printk("#syscall %s, fd= %d, ptr= %lu, dir= %d\n","lseek", fd, ptr, dir);
  // printk("sys lseek at fd %d, at offset %ld, beingging from DIR %d\n",fd,ptr,dir);
  ssize_t r = -EBADF;
  file_t* f = file_get(fd);

  if (f)
  {
    r = file_lseek(f, ptr, dir);
    file_decref(f);
    // printk("lseek returns %ld\n",r);
  }

  return r;
}

long sys_lstat(const char* name, void* st)
{
  // struct frontend_stat buf;
  // size_t name_size = strlen(name)+1;
  // long ret = frontend_syscall(SYS_lstat, va2pa(name), name_size, va2pa(&buf), 0, 0, 0, 0);
  // copy_stat(st, &buf);
  // return ret;
  return -EBADF;
}

long sys_fstatat(int dirfd, const char* name, void* st, int flags)
{
  // printk("#syscall %s, dirfd= %d, name= %s, st= %p, flags= %d\n","fstatat", dirfd, name,st, flags);
  int kfd = at_kfd(dirfd);
  if (kfd != -1) {
    // flags are ignored.
    int ret = file_statat(dirfd, name, (struct stat *)st);
  // printk("#syscall fstatat success and returns %ld\n", ret);
  // file_display();
    return ret;
  }
  return -EBADF;
}

long sys_stat(const char* name, void* st)
{
  return sys_fstatat(AT_FDCWD, name, st, 0);
}

long sys_faccessat(int dirfd, const char *name, int mode)
{
  // printk("#syscall %s, dirfd= %d, name= %s, mode= %d\n","faccessat", dirfd, name, mode);
  // printk("faccessat %s, with mode %d\n",name,mode);
  int kfd = at_kfd(dirfd);
  if (kfd != -1) {
    size_t name_size = strlen(name)+1;
    // printk("#syscall faccessat returns %ld\n",0l);
    return file_accessat(kfd,name,mode);
  }
  // return -EBADF;
  // return -EINVAL;
  return -EBADF;
}

long sys_access(const char *name, int mode)
{
  return sys_faccessat(AT_FDCWD, name, mode);
}

long sys_linkat(int old_dirfd, const char* old_name, int new_dirfd, const char* new_name, int flags)
{
  // int old_kfd = at_kfd(old_dirfd);
  // int new_kfd = at_kfd(new_dirfd);
  // if (old_kfd != -1 && new_kfd != -1) {
  //   size_t old_size = strlen(old_name)+1;
  //   size_t new_size = strlen(new_name)+1;
  //   return frontend_syscall(SYS_linkat, old_kfd, va2pa(old_name), old_size,
  //                                       new_kfd, va2pa(new_name), new_size,
  //                                       flags);
  // }
  return -EBADF;
}

long sys_link(const char* old_name, const char* new_name)
{
  return sys_linkat(AT_FDCWD, old_name, AT_FDCWD, new_name, 0);
}

long sys_unlinkat(int dirfd, const char* name, int flags)
{
  // printk("#syscall %s, dirfd= %d, name= %s, flags= %d\n","unlinkat", dirfd, name, flags);
  int kfd = at_kfd(dirfd);
  if (kfd != -1) {
    size_t name_size = strlen(name)+1;
    int ret = file_unlinkat(dirfd,name,flags);
    // file_display();
    return ret;
  }
  return -EBADF;
}

long sys_unlink(const char* name)
{
  return sys_unlinkat(AT_FDCWD, name, 0);
}

long sys_mkdirat(int dirfd, const char* name, int mode)
{
  // int kfd = at_kfd(dirfd);
  // if (kfd != -1) {
  //   size_t name_size = strlen(name)+1;
  //   return frontend_syscall(SYS_mkdirat, kfd, va2pa(name), name_size, mode, 0, 0, 0);
  // }
  return -EBADF;
}

long sys_mkdir(const char* name, int mode)
{
  return sys_mkdirat(AT_FDCWD, name, mode);
}

uintptr_t sys_getcwd(char* buf, size_t size)
{
  // printk("#syscall %s, buf= %p, size= %lu\n","getcwd", buf, size);
  populate_mapping(buf, size, PROT_WRITE);
  // return frontend_syscall(SYS_getcwd, va2pa(buf), size, 0, 0, 0, 0, 0);
  // return -EBADF;
  return (uintptr_t)file_getcwd(buf,size);
}

size_t sys_brk(size_t pos)
{
  return do_brk(pos);
}

int sys_uname(void* buf)
{
  const int sz = 65;
  strcpy(buf + 0*sz, "Proxy Kernel");
  strcpy(buf + 1*sz, "");
  strcpy(buf + 2*sz, "3.4.5");
  strcpy(buf + 3*sz, "");
  strcpy(buf + 4*sz, "");
  strcpy(buf + 5*sz, "");
  return 0;
}

pid_t sys_getpid()
{
  return 0;
}

int sys_getuid()
{
  return 0;
}

uintptr_t sys_mmap(uintptr_t addr, size_t length, int prot, int flags, int fd, off_t offset)
{
  uintptr_t ret =  do_mmap(addr, length, prot, flags, fd, offset);
  return ret;
}

int sys_munmap(uintptr_t addr, size_t length)
{
  return do_munmap(addr, length);
}

uintptr_t sys_mremap(uintptr_t addr, size_t old_size, size_t new_size, int flags)
{
  return do_mremap(addr, old_size, new_size, flags);
}

uintptr_t sys_mprotect(uintptr_t addr, size_t length, int prot)
{
  return do_mprotect(addr, length, prot);
}

int sys_rt_sigaction(int sig, const void* act, void* oact, size_t sssz)
{
  if (oact)
    memset(oact, 0, sizeof(long) * 3);

  return 0;
}

long sys_time(long* loc)
{
  uintptr_t t = rdcycle() / CLOCK_FREQ;
  if (loc)
    *loc = t;
  return t;
}

int sys_times(long* loc)
{
  uintptr_t t = rdcycle();
  kassert(CLOCK_FREQ % 1000000 == 0);
  loc[0] = t / (CLOCK_FREQ / 1000000);
  loc[1] = 0;
  loc[2] = 0;
  loc[3] = 0;
  
  return 0;
}

int sys_gettimeofday(long* loc)
{
  uintptr_t t = rdcycle();
  loc[0] = t / CLOCK_FREQ;
  loc[1] = (t % CLOCK_FREQ) / (CLOCK_FREQ / 1000000);
  
  return 0;
}

ssize_t sys_writev(int fd, const long* iov, int cnt)
{
  ssize_t ret = 0;
  for (int i = 0; i < cnt; i++)
  {
    ssize_t r = sys_write(fd, (void*)iov[2*i], iov[2*i+1]);
    if (r < 0)
      return r;
    ret += r;
  }
  return ret;
}

int sys_chdir(const char *path)
{
  // return frontend_syscall(SYS_chdir, va2pa(path), 0, 0, 0, 0, 0, 0);
  // return -EBADF;
  return file_chdir(path);
}

int sys_getdents(int fd, void* dirbuf, int count)
{
  return 0; //stub
}

static int sys_stub_success()
{
  return 0;
}

static int sys_stub_nosys()
{
  return -ENOSYS;
}

long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, unsigned long n)
{
  const static void* syscall_table[] = {
    [SYS_exit] = sys_exit,
    [SYS_exit_group] = sys_exit,
    [SYS_read] = sys_read,
    [SYS_pread] = sys_pread,
    [SYS_write] = sys_write,
    [SYS_openat] = sys_openat,
    [SYS_close] = sys_close,
    [SYS_fstat] = sys_fstat,
    [SYS_lseek] = sys_lseek,
    [SYS_fstatat] = sys_fstatat,
    // [SYS_linkat] = sys_linkat,
    [SYS_unlinkat] = sys_unlinkat,
    // [SYS_mkdirat] = sys_mkdirat,
    // [SYS_renameat] = sys_renameat,
    [SYS_getcwd] = sys_getcwd,
    [SYS_brk] = sys_brk,
    [SYS_uname] = sys_uname,
    [SYS_getpid] = sys_getpid,
    [SYS_getuid] = sys_getuid,
    [SYS_geteuid] = sys_getuid,
    [SYS_getgid] = sys_getuid,
    [SYS_getegid] = sys_getuid,
    [SYS_mmap] = sys_mmap,
    [SYS_munmap] = sys_munmap,
    [SYS_mremap] = sys_mremap,
    [SYS_mprotect] = sys_mprotect,
    [SYS_rt_sigaction] = sys_rt_sigaction,
    [SYS_gettimeofday] = sys_gettimeofday,
    [SYS_times] = sys_times,
    [SYS_writev] = sys_writev,
    [SYS_faccessat] = sys_faccessat,
    [SYS_fcntl] = sys_fcntl,
    // [SYS_ftruncate] = sys_ftruncate,
    [SYS_getdents] = sys_getdents,
    // [SYS_dup] = sys_dup,
    [SYS_readlinkat] = sys_stub_nosys,
    [SYS_rt_sigprocmask] = sys_stub_success,
    [SYS_ioctl] = sys_stub_nosys,
    [SYS_clock_gettime] = sys_stub_nosys,
    [SYS_getrusage] = sys_stub_nosys,
    [SYS_getrlimit] = sys_stub_nosys,
    [SYS_setrlimit] = sys_stub_nosys,
    [SYS_chdir] = sys_chdir,
  };

  const static void* old_syscall_table[] = {
    [-OLD_SYSCALL_THRESHOLD + SYS_open] = sys_open,
    // [-OLD_SYSCALL_THRESHOLD + SYS_link] = sys_link,
    // [-OLD_SYSCALL_THRESHOLD + SYS_unlink] = sys_unlink,
    // [-OLD_SYSCALL_THRESHOLD + SYS_mkdir] = sys_mkdir,
    // [-OLD_SYSCALL_THRESHOLD + SYS_access] = sys_access,
    // [-OLD_SYSCALL_THRESHOLD + SYS_stat] = sys_stat,
    // [-OLD_SYSCALL_THRESHOLD + SYS_lstat] = sys_lstat,
    [-OLD_SYSCALL_THRESHOLD + SYS_time] = sys_time,
  };

  syscall_t f = 0;

  if (n < ARRAY_SIZE(syscall_table))
    f = syscall_table[n];
  else if (n - OLD_SYSCALL_THRESHOLD < ARRAY_SIZE(old_syscall_table))
    f = old_syscall_table[n - OLD_SYSCALL_THRESHOLD];

  // printk("## syscall %ld called! ##\n",n);

  if (!f)
    panic("bad syscall #%ld!",n);

  return f(a0, a1, a2, a3, a4, a5, n);
}
