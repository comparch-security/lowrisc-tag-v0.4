// See LICENSE for license details.

#include "sim.h"
#include "mmu.h"
#include "htif.h"
#include "cachesim.h"
#include "tagcachesim.h"
#include "extension.h"
#include "tag.h"
#include <dlfcn.h>
#include <fesvr/option_parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <vector>
#include <string>
#include <memory>

static void help()
{
  fprintf(stderr, "usage: spike [host options] <target program> [target options]\n");
  fprintf(stderr, "Host Options:\n");
  fprintf(stderr, "  -p<n>                  Simulate <n> processors [default 1]\n");
  fprintf(stderr, "  -m<n>                  Provide <n> MiB of target memory [default 4096]\n");
  fprintf(stderr, "  -t<n>                  number of tag bits\n");
  fprintf(stderr, "  -r<n>                  Record address and disasm last <n> instructions each processor executed\n");
  fprintf(stderr, "  -d                     Interactive debug mode\n");
  fprintf(stderr, "  -g                     Track histogram of PCs\n");
  fprintf(stderr, "  -l                     Generate a log of execution\n");
  fprintf(stderr, "  -h                     Print this help message\n");
  fprintf(stderr, "  --pfcs=<n>             Skip <n> instructions to warm up cache before enabling performance counter\n");
  fprintf(stderr, "  --pfcn=<n>             Total number of instructions spike will execute after cache warming up\n");
  fprintf(stderr, "  --isa=<name>           RISC-V ISA string [default %s]\n", DEFAULT_ISA);
  fprintf(stderr, "  --ic=<S>:<W>:<B>       Instantiate a cache model with S sets,\n");
  fprintf(stderr, "  --dc=<S>:<W>:<B>         W ways, and B-byte blocks (with S and\n");
  fprintf(stderr, "  --l2=<S>:<W>:<B>         B both powers of 2).\n");
  fprintf(stderr, "  --utc=<S>:<W>:<B>:<WB> Enable T-bit tags with a unified tag cache\n");
  fprintf(stderr, "  --tt=<S>:<W>:<B>:<WB>  Enable T-bit tags with a separate tag cache\n");
  fprintf(stderr, "  --tm0=<S>:<W>:<B>:<WB> Use separate tag map 0\n");
  fprintf(stderr, "  --tm1=<S>:<W>:<B>:<WB> Use separate tag map 1\n");
  fprintf(stderr, "  --extension=<name>     Specify RoCC Extension\n");
  fprintf(stderr, "  --extlib=<name>        Shared library to load\n");
  fprintf(stderr, "  --dump-config-string   Print platform configuration string and exit\n");
  exit(1);
}

int main(int argc, char** argv)
{
  bool debug = false;
  bool histogram = false;
  bool log = false;
  size_t nc_insn_trace = 0; //record 0 insn as default
  size_t pfc_skip = 0;
  size_t pfc_nc = 0;
  bool dump_config_string = false;
  size_t nprocs = 1;
  size_t mem_mb = 0;
  size_t tagsz = 4;             // set to 4-bit tag by default
  std::unique_ptr<icache_sim_t> ic;
  std::unique_ptr<dcache_sim_t> dc;
  std::unique_ptr<cache_sim_t> l2;
  std::unique_ptr<tag_cache_sim_t> utc, tt, tm0, tm1;
  std::string utc_cfg, tt_cfg, tm0_cfg, tm1_cfg;
  std::function<extension_t*()> extension;
  const char* isa = DEFAULT_ISA;

  option_parser_t parser;
  parser.help(&help);
  parser.option('h', 0, 0, [&](const char* s){help();});
  parser.option('d', 0, 0, [&](const char* s){debug = true;});
  parser.option('g', 0, 0, [&](const char* s){histogram = true;});
  parser.option('l', 0, 0, [&](const char* s){log = true;});
  parser.option('p', 0, 1, [&](const char* s){nprocs = atoi(s);});
  parser.option('m', 0, 1, [&](const char* s){mem_mb = atoi(s);});
  parser.option('t', 0, 1, [&](const char* s){tagsz = atoi(s);});
  parser.option('r',0,1,[&](const char* s){nc_insn_trace=atoi(s);});
  parser.option(0,"pfcs",1, [&](const char* s){pfc_skip=atoi(s);});
  parser.option(0,"pfcn",1, [&](const char* s){pfc_nc=atoi(s);});
  parser.option(0, "ic", 1, [&](const char* s){ic.reset(new icache_sim_t(s));});
  parser.option(0, "dc", 1, [&](const char* s){dc.reset(new dcache_sim_t(s));});
  parser.option(0, "l2", 1, [&](const char* s){l2.reset(cache_sim_t::construct(s, "L2$"));});
  parser.option(0, "tt", 1, [&](const char* s){tt_cfg = std::string(s);});
  parser.option(0, "tm0", 1, [&](const char* s){tm0_cfg = std::string(s);});
  parser.option(0, "tm1", 1, [&](const char* s){tm1_cfg = std::string(s);});
  parser.option(0, "utc", 1, [&](const char* s){utc_cfg = std::string(s);});
  parser.option(0, "isa", 1, [&](const char* s){isa = s;});
  parser.option(0, "extension", 1, [&](const char* s){extension = find_extension(s);});
  parser.option(0, "dump-config-string", 0, [&](const char *s){dump_config_string = true;});
  parser.option(0, "extlib", 1, [&](const char *s){
    void *lib = dlopen(s, RTLD_NOW | RTLD_GLOBAL);
    if (lib == NULL) {
      fprintf(stderr, "Unable to load extlib '%s': %s\n", s, dlerror());
      exit(-1);
    }
  });

  auto argv1 = parser.parse(argv);
  std::vector<std::string> htif_args(argv1, (const char*const*)argv + argc);
  sim_t s(isa, nprocs, mem_mb, tagsz, htif_args);

  tg::record_mem(DRAM_BASE, s.get_memsz(), 64, tagsz);
  if(tt_cfg.size())
    tt.reset(sep_tag_cache_sim_t::construct(tt_cfg.c_str(), "TT$", &s, 0, tagsz));
  if(tt_cfg.size() && tm0_cfg.size())
    tm0.reset(sep_tag_cache_sim_t::construct(tm0_cfg.c_str(), "TM0$", &s, 1, 1));
  if(tt_cfg.size() && tm0_cfg.size() && tm1_cfg.size())
    tm1.reset(sep_tag_cache_sim_t::construct(tm1_cfg.c_str(), "TM1$", &s, 2, 1));
  if(!tt_cfg.size() && utc_cfg.size())
    utc.reset(uni_tag_cache_sim_t::construct(utc_cfg.c_str(), "UTC$", &s, tagsz));

  tg::init();

  if (dump_config_string) {
    printf("%s", s.get_config_string());
    return 0;
  }

  if (!*argv1)
    help();

  if (ic && l2) ic->set_miss_handler(&*l2);
  if (dc && l2) dc->set_miss_handler(&*l2);
  for (size_t i = 0; i < nprocs; i++)
  {
    if (ic) s.get_core(i)->get_mmu()->register_memtracer(&*ic);
    if (dc) s.get_core(i)->get_mmu()->register_memtracer(&*dc);
    if (extension) s.get_core(i)->register_extension(extension());
  }

  // tag cache
  if(l2) {
    if(tt) l2->set_miss_handler(&*tt);
    else if(utc) l2->set_miss_handler(&*utc);
  } else {
    if(ic && tt) ic->set_miss_handler(&*tt);
    if(dc && tt) dc->set_miss_handler(&*tt);
    if(ic && !tt && utc) ic->set_miss_handler(&*utc);
    if(dc && !tt && utc) dc->set_miss_handler(&*utc);
  }
  if(tt && tm0) tt->set_tag_map(&*tm0);
  if(tt && tm0 && tm1) tm0->set_tag_map(&*tm1);

  s.set_debug(debug);
  s.set_log(log);
  s.set_histogram(histogram);
  s.set_nc_insn_trace(nc_insn_trace);
  s.set_pfc_skip(pfc_skip);
  s.set_pfc_nc(pfc_nc);
  return s.run();
}
