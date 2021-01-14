// See LICENSE for license details.

#ifndef _BBL_H
#define _BBL_H

#ifndef __ASSEMBLER__

#include "encoding.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

typedef struct {
  int phent;
  int phnum;
  int is_supervisor;
  size_t phdr;
  size_t phdr_size;
  size_t bias;
  size_t entry;
  size_t brk_min;
  size_t brk;
  size_t brk_max;
  size_t mmap_max;
  size_t stack_top;
  size_t t0;
  size_t instret0;
} elf_info;

extern elf_info current;

void load_elf(const char* fn, elf_info* info);
int load_elf_from_DRAM(void* blob, size_t size, elf_info* info);

typedef struct
{
  long gpr[32];
  long status;
  long epc;
  long badvaddr;
  long cause;
  long insn;
} trapframe_t;

#define panic(s,...) do { do_panic(s"\n", ##__VA_ARGS__); } while(0)
#define kassert(cond) do { if(!(cond)) kassert_fail(""#cond); } while(0)
void do_panic(const char* s, ...) __attribute__((noreturn));
void kassert_fail(const char* s) __attribute__((noreturn));
void die(int) __attribute__((noreturn));

#ifdef __cplusplus
extern "C" {
#endif

void printk(const char* s, ...);
void printm(const char* s, ...);
// int vsnprintf(char* out, size_t n, const char* s, va_list vl);
// int snprintf(char* out, size_t n, const char* s, ...);
void start_user(trapframe_t* tf) __attribute__((noreturn));
void dump_tf(trapframe_t*);
void dump_uarch_counters();

static inline int insn_len(long insn)
{
  return (insn & 0x3) < 0x3 ? 2 : 4;
}

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

#ifdef __cplusplus
}
#endif

#endif // !__ASSEMBLER__

#endif
