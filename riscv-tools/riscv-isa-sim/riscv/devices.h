#ifndef _RISCV_DEVICES_H
#define _RISCV_DEVICES_H

#include "decode.h"
#include <map>
#include <vector>

class processor_t;

class abstract_device_t {
 public:
  virtual bool load(word_t addr, size_t len, uint8_t* bytes) = 0;
  virtual bool store(word_t addr, size_t len, const uint8_t* bytes) = 0;
  virtual ~abstract_device_t() {}
};

class bus_t : public abstract_device_t {
 public:
  bool load(word_t addr, size_t len, uint8_t* bytes);
  bool store(word_t addr, size_t len, const uint8_t* bytes);
  void add_device(word_t addr, abstract_device_t* dev);

 private:
  std::map<word_t, abstract_device_t*> devices;
};

class rom_device_t : public abstract_device_t {
 public:
  rom_device_t(std::vector<char> data);
  bool load(word_t addr, size_t len, uint8_t* bytes);
  bool store(word_t addr, size_t len, const uint8_t* bytes);
  const std::vector<char>& contents() { return data; }
 private:
  std::vector<char> data;
};

class rtc_t : public abstract_device_t {
 public:
  rtc_t(std::vector<processor_t*>&);
  bool load(word_t addr, size_t len, uint8_t* bytes);
  bool store(word_t addr, size_t len, const uint8_t* bytes);
  size_t size() { return regs.size() * sizeof(regs[0]); }
  void increment(word_t inc);
 private:
  std::vector<processor_t*>& procs;
  std::vector<uint64_t> regs;
  uint64_t time() { return regs[0]; }
};

#endif
