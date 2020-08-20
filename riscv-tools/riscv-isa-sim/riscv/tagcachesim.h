// See LICENSE for license details.

#ifndef _RISCV_TAG_CACHE_SIM_H
#define _RISCV_TAG_CACHE_SIM_H

#include "memtracer.h"
#include "cachesim.h"
#include "sim.h"
#include <string>

class tag_cache_sim_t : public cache_sim_t
{
 public:
  tag_cache_sim_t(size_t sets, size_t ways, size_t linesz, uint8_t wb, const char* name, sim_t* sim, int level);
  tag_cache_sim_t(const tag_cache_sim_t& rhs);
  virtual ~tag_cache_sim_t();
  virtual uint64_t access(uint64_t addr, size_t bytes, bool store) = 0;
  void set_tag_map(tag_cache_sim_t * tm) { tag_map = tm; }

 protected:
  int level;
  bool wb_enforce;

  uint8_t *datas;
  uint8_t *created;
  tag_cache_sim_t* tag_map;

  sim_t *sim;

  void init();

  static const uint64_t TAGFLAG = 1ULL << 61;
  uint64_t* check_tag(uint64_t addr, size_t &row);
  uint64_t* victimize(uint64_t addr, size_t &row);
  uint64_t subrow(uint64_t addr) { return addr & (linesz-7); }
  void refill(uint64_t addr, size_t row);
  void writeback(size_t row);
  size_t memsz() { return sim->memsz; }
  uint64_t read_mem(uint64_t addr) {
    return *(uint64_t *)(sim->addr_to_mem(addr & ~(uint64_t)(0x7)));
  }

  uint64_t read(uint64_t addr, uint64_t &data, uint8_t fetch, uint8_t writeacc);
  uint64_t write(uint64_t addr, uint64_t data, uint64_t mask);
  uint64_t create(uint64_t addr, uint64_t data, uint64_t mask);
};

class sep_tag_cache_sim_t : public tag_cache_sim_t
{
 public:
  sep_tag_cache_sim_t(size_t sets, size_t ways, size_t linesz, uint8_t wb, const char* name, sim_t* sim, int level)
   : tag_cache_sim_t(sets, ways, linesz, wb, name, sim, level) {}
  static tag_cache_sim_t* construct(const char* config, const char* name, sim_t* sim, int level, size_t tagsz);
  virtual uint64_t access(uint64_t addr, size_t bytes, bool store);
};

class uni_tag_cache_sim_t : public tag_cache_sim_t
{
 public:
  uni_tag_cache_sim_t(size_t sets, size_t ways, size_t linesz, uint8_t wb, const char* name, sim_t* sim)
   : tag_cache_sim_t(sets, ways, linesz, wb, name, sim, 0) {}
  static tag_cache_sim_t* construct(const char* config, const char* name, sim_t* sim, size_t tagsz);
  virtual uint64_t access(uint64_t addr, size_t bytes, bool store);
};


#endif
