// See LICENSE for license details.

#ifndef _RISCV_TAG_H
#define _RISCV_TAG_H

#include<cstdlib>
#include<vector>

class tg {
 public:
  typedef struct {
    uint64_t base;              // base of this level
    uint64_t size;              // siz eof this level
    uint64_t tagsz;             // tag size
    uint64_t linesz;            // block size of this level
    uint64_t base_pre;          // base of previous level
    uint64_t linesz_pre;        // block size of previous level
    uint64_t ratio;             // mapping ratio
    uint64_t aoffset;           // offset used in address calculation
    uint64_t moffset;           // offset used in mask calculation
    uint64_t dmask;             // write data mask
    uint64_t amask;             // address mask for write mask calculation
  } data_t;

private:
  static uint64_t memsz;
  static uint64_t membase;
  static uint64_t wordsz;
  static std::vector<data_t> data;

  static uint64_t ilog2(uint64_t d) { uint64_t rv = 0; while(d >>= 1) rv++; return rv;}

 public:
  static uint64_t tagbase;

  static void record_mem(uint64_t membase_, uint64_t memsize_, uint64_t wordsz_, uint64_t tagsz_) {
    membase = membase_;
    memsz = memsize_;
    wordsz = wordsz_;
    if(data.size() == 0) data.resize(1);
    data[0].tagsz = tagsz_;
  }

  static void record_tc(unsigned int level, uint64_t tagsz, uint64_t linesz) {
    if(data.size() <= level) data.resize(level+1);
    data[level].tagsz = tagsz;
    data[level].linesz = linesz;
  }

  static void init() {
    for(unsigned int i=0; i<data.size(); i++) {
      data[i].base_pre = i==0 ? membase : data[i-1].base;
      data[i].linesz_pre = i==0 ? wordsz / 8 : data[i-1].linesz;
      data[i].ratio = data[i].linesz_pre * 8 / data[i].tagsz;
      data[i].size = (i==0 ? memsz : data[i-1].size)  / data[i].ratio;
      data[i].base = membase + memsz - data[i].size;
      data[i].aoffset = ilog2(data[i].ratio);
      data[i].moffset = ilog2(data[i].linesz_pre);
      data[i].dmask = ((uint64_t)1 << data[i].tagsz) - 1;
      data[i].amask = 64 / data[i].tagsz - 1;
    }
    tagbase = data[0].base;
  }

  static uint64_t addr_conv(int i, uint64_t addr) {
    return data[i].base + (((addr - data[i].base_pre) >> data[i].aoffset) & ~0x7);
  }
  static uint64_t tag_offset(int i, uint64_t addr) {
    return ((addr >> data[i].moffset) & data[i].amask) * data[i].tagsz;
  }
  static uint64_t mask(int i, uint64_t addr, size_t bytes) {
    return (bytes <= 8 ? data[i].dmask :  (((uint64_t)1 << data[i].tagsz*(bytes/8)) - 1)) << tag_offset(i, addr);
  }
  static uint64_t extract_tag(int i, uint64_t addr, uint64_t tag) {
    return (tag >> tag_offset(i, addr)) & data[i].dmask;
  }
  static uint64_t addr_conv_rev(int i, uint64_t addr) {
    return data[i].base_pre + ((addr - data[i].base) << data[i].aoffset);
  }
  static bool is_top(uint64_t addr) {
    return addr >= data[data.size()-1].base;
  }
};

#endif
