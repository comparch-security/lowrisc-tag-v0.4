// See LICENSE for license details.

#ifndef __SYSCALL_H
#define __SYSCALL_H

#include "device.h"
#include <vector>
#include <string>
#include <map>

class syscall_t;
typedef word_t (syscall_t::*syscall_func_t)(word_t, word_t, word_t, word_t, word_t, word_t, word_t);

class htif_t;
class memif_t;

class fds_t
{
 public:
  word_t alloc(int fd);
  void dealloc(word_t fd);
  int lookup(word_t fd);
 private:
  std::vector<int> fds;
};

class syscall_t : public device_t
{
 public:
  syscall_t(htif_t*);
  
 private:
  const char* identity() { return "syscall_proxy"; }

  htif_t* htif;
  memif_t* memif;
  std::vector<syscall_func_t> table;
  fds_t fds;

  void handle_syscall(command_t cmd);
  void dispatch(addr_t mm);
  std::string do_chroot(const char* fn);
  std::string undo_chroot(const char* fn);

  word_t sys_exit(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_openat(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_read(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_pread(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_write(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_pwrite(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_close(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_lseek(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_fstat(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_lstat(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_fstatat(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_faccessat(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_fcntl(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_ftruncate(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_renameat(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_linkat(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_unlinkat(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_mkdirat(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_getcwd(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_getmainvars(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
  word_t sys_chdir(word_t, word_t, word_t, word_t, word_t, word_t, word_t);
};

#endif
