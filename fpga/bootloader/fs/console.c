#include "bbl.h"
// #include "frontend.h"
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "sbi.h"
#include "mcall.h"

extern int vsnprintf(char* out, size_t n, const char* s, va_list vl);

extern int snprintf(char* out, size_t n, const char* s, ...);

void sbi_putchar_emulate(unsigned int ch)
{
  asm volatile ("mv a0, %0; li a7, 1; ecall;" ::"r"(ch));
}

static void vprintk(const char* s, va_list vl)
{
  char out[256]; // XXX
  int res = vsnprintf(out, sizeof(out), s, vl);
  sbi_send_buf(out, res < sizeof(out) ? res : sizeof(out));
}

void printk(const char* s, ...)
{
  va_list vl;
  va_start(vl, s);

  vprintk(s, vl);

  va_end(vl);
}


void dump_tf(trapframe_t* tf)
{
  static const char* regnames[] = {
    "z ", "ra", "sp", "gp", "tp", "t0",  "t1",  "t2",
    "s0", "s1", "a0", "a1", "a2", "a3",  "a4",  "a5",
    "a6", "a7", "s2", "s3", "s4", "s5",  "s6",  "s7",
    "s8", "s9", "sA", "sB", "t3", "t4",  "t5",  "t6"
  };

  tf->gpr[0] = 0;

  for(int i = 0; i < 32; i+=4)
  {
    for(int j = 0; j < 4; j++)
      printk("%s %lx%c",regnames[i+j],tf->gpr[i+j],j < 3 ? ' ' : '\n');
  }
  printk("pc %lx va %lx insn       %x sr %lx\n", tf->epc, tf->badvaddr,
         (uint32_t)tf->insn, tf->status);
}

void die(int code)
{
  sbi_shutdown();
  while (1);
}


void do_panic(const char* s, ...)
{
  va_list vl;
  va_start(vl, s);

  vprintk(s, vl);
  die(-1);

  va_end(vl);
}

void kassert_fail(const char* s)
{
  register uintptr_t ra asm ("ra");
  do_panic("assertion failed @ %p: %s\n", ra, s);
}
