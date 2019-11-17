#include "devices.h"

void bus_t::add_device(word_t addr, abstract_device_t* dev)
{
  devices[-addr] = dev;
}

bool bus_t::load(word_t addr, size_t len, uint8_t* bytes)
{
  auto it = devices.lower_bound(-addr);
  if (it == devices.end())
    return false;
  return it->second->load(addr - -it->first, len, bytes);
}

bool bus_t::store(word_t addr, size_t len, const uint8_t* bytes)
{
  auto it = devices.lower_bound(-addr);
  if (it == devices.end())
    return false;
  return it->second->store(addr - -it->first, len, bytes);
}
