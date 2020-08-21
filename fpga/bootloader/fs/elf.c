// See LICENSE for license details.

#include "vm.h"
#include "bbl.h"
#include "mtrap.h"
#include "bits.h"
#include <sys/stat.h>
#include <fcntl.h>
#include "elf.h"
#include <string.h>

#define set_tagged_val(dst, val, tag) \
  asm volatile ( "tagw %0, %1; sw %0, 0(%2); tagw %0, zero;" : : "r" (val), "r" (tag), "r" (dst))
#define disable_tag_rules() write_csr(stagctrl, 0)
#define enable_tag_prop() write_csr(stagctrl, TMASK_STORE_PROP | TMASK_LOAD_PROP | TMASK_ALU_PROP )
#define get_tag(dst) ({\
  unsigned long __tmp1, __tmp2;\
  asm volatile ( "lw %0, 0(%2); tagr %1, %0; tagw %0, zero;" :"=r"(__tmp1), "=r"(__tmp2):"r"(dst));\
  __tmp2;})


typedef struct  {
    uint64_t addr;
    uint64_t tag;
} Tag;

void load_elf(const char* fn, elf_info* info)
{
  file_t* file = file_open(fn, O_RDONLY);
  if (!file)
    goto fail;

  Elf_Ehdr eh;
  ssize_t ehdr_size = file_pread(file, &eh, sizeof(eh), 0);
  if (ehdr_size < (ssize_t)sizeof(eh) ||
      !(eh.e_ident[0] == '\177' && eh.e_ident[1] == 'E' &&
        eh.e_ident[2] == 'L'    && eh.e_ident[3] == 'F'))
    goto fail;

#ifdef __riscv64
  assert(IS_ELF64(eh));
#else
  assert(IS_ELF32(eh));
#endif

  uintptr_t min_vaddr = -1;
  size_t phdr_size = eh.e_phnum * sizeof(Elf_Phdr);
  if (phdr_size > info->phdr_size)
    goto fail;
  ssize_t ret = file_pread(file, (void*)info->phdr, phdr_size, eh.e_phoff);
  if (ret < (ssize_t)phdr_size)
    goto fail;
  info->phnum = eh.e_phnum;
  info->phent = sizeof(Elf_Phdr);
  Elf_Phdr* ph = (typeof(ph))info->phdr;
  for (int i = 0; i < eh.e_phnum; i++)
    if (ph[i].p_type == PT_LOAD && ph[i].p_memsz && ph[i].p_vaddr < min_vaddr)
      min_vaddr = ph[i].p_vaddr;
  min_vaddr = ROUNDDOWN(min_vaddr, RISCV_PGSIZE);
  uintptr_t bias = 0;
  if (eh.e_type == ET_DYN)
    bias = first_free_paddr - min_vaddr;
  min_vaddr += bias;
  info->entry = eh.e_entry + bias;
  int flags = MAP_FIXED | MAP_PRIVATE;
  for (int i = eh.e_phnum - 1; i >= 0; i--) {
    if(ph[i].p_type == PT_LOAD && ph[i].p_memsz) {
      uintptr_t prepad = ph[i].p_vaddr % RISCV_PGSIZE;
      uintptr_t vaddr = ph[i].p_vaddr + bias;
      if (vaddr + ph[i].p_memsz > info->brk_min)
        info->brk_min = vaddr + ph[i].p_memsz;
      int flags2 = flags | (prepad ? MAP_POPULATE : 0);
      if (__do_mmap(vaddr - prepad, ph[i].p_filesz + prepad, -1, flags2, file, ph[i].p_offset - prepad) != vaddr - prepad)
        goto fail;
      // printk("mapping vaddr\n %p-%p\n",
      //   vaddr-prepad,vaddr+ ph[i].p_filesz );
      memset((void*)vaddr - prepad, 0, prepad);
      size_t mapped = ROUNDUP(ph[i].p_filesz + prepad, RISCV_PGSIZE) - prepad;
      if (ph[i].p_memsz > mapped){
        if (__do_mmap(vaddr + mapped, ph[i].p_memsz - mapped, -1, flags|MAP_ANONYMOUS, 0, 0) != vaddr + mapped)
          goto fail;
        // printk("mapping vaddr\n %p-%p\n",
        //   vaddr+mapped, vaddr+ph[i].p_memsz);
      }
    }
  }

#ifdef __riscv64
  int en_tagging = 1; // reserved for cmd options control.
  int en_tagchk = 0; //reserved for cmd options control.
  if (en_tagging){
    if (eh.e_shoff != 0 && eh.e_shnum != 0){
      Elf64_Shdr sh;
      Elf64_Shdr shstr;
      size_t shtagndx = eh.e_shnum;
      uintptr_t sec_str = 0;
      uintptr_t sec_tag = 0;
      const char * mmap_str = 0;
      const char * tag_str_ref = ".tag";
      Tag * mmap_tag = NULL;
      size_t ntags = 0;
      extern uintptr_t __do_brk(size_t);
      uintptr_t brkmin = __do_brk(0);

      if (eh.e_shstrndx != 0){
        ret = file_pread(file,(void*)&shstr,sizeof(Elf64_Shdr),eh.e_shoff + (eh.e_shstrndx * eh.e_shentsize));
        if (ret < (typeof(ret))sizeof(Elf64_Shdr))
          goto fail;
        sec_str = __do_mmap(0,shstr.sh_size,PROT_READ,MAP_PRIVATE,file,shstr.sh_offset);
        if (sec_str == (uintptr_t)-1)
          goto fail;
        mmap_str = (const char *) sec_str;
        
        for (size_t i = 0 ; i < eh.e_shnum ; i++){
          ret = file_pread(file,(void*)&sh,sizeof(Elf64_Shdr),eh.e_shoff + (i * eh.e_shentsize));
          if (ret < (typeof(ret))sizeof(Elf64_Shdr))
            goto fail;

          if (strcmp((mmap_str + sh.sh_name),tag_str_ref) == 0) {
            shtagndx = i;
            break;
          }
          
        }

        if (shtagndx < eh.e_shnum){
          sec_tag = __do_mmap(0,sh.sh_size,PROT_READ,MAP_PRIVATE,file,sh.sh_offset);
          if (sec_tag == (uintptr_t)-1)
            goto fail;

          mmap_tag = (Tag *) sec_tag;
          ntags = sh.sh_size/sizeof(Tag);

          disable_tag_rules();
          enable_tag_prop();

          // printk("stagctrl = %#x\n",read_csr(stagctrl));

          for (size_t i = 0; i < ntags ; i++){
            if (mmap_tag[i].addr == (uintptr_t)-1) break;
            set_tagged_val(mmap_tag[i].addr,*(uint64_t*)mmap_tag[i].addr,mmap_tag[i].tag);
            // printk("tagging insn no.%d @ %p with tag %x,",i,mmap_tag[i].addr,mmap_tag[i].tag);
            // printk("tag is %x\n",get_tag(mmap_tag[i].addr));
          }
          
        }

      }      
    }
  }

#endif


  file_decref(file);
  return;

fail:
  panic("couldn't open ELF program: %s!", fn);
}
