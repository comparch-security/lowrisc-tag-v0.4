// See LICENSE for license details.

#include "tagcachesim.h"
#include <cassert>
#include <cstring>
#include <cassert>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#define TC_STAT_BEHAV

// definitions for the global static tag helper class

uint64_t tg::membase;
uint64_t tg::memsz;
uint64_t tg::wordsz;
uint64_t tg::tagbase;
std::vector<tg::data_t> tg::data;


// ----------------- tag cache base class ----------------------- //

static uint8_t *empty_block = NULL;

tag_cache_sim_t::tag_cache_sim_t(size_t sets, size_t ways, size_t linesz, uint8_t wb, const char* name, sim_t* sim, int level)
  : cache_sim_t(sets, ways, linesz, name), level(level), wb_enforce(wb), sim(sim)
{
  init();
}

tag_cache_sim_t::tag_cache_sim_t(const tag_cache_sim_t& rhs)
  : cache_sim_t(rhs), level(rhs.level), wb_enforce(rhs.wb_enforce), sim(rhs.sim)
{
  init();
  memcpy(datas, rhs.datas, sets*ways*linesz*sizeof(uint8_t));
}

tag_cache_sim_t::~tag_cache_sim_t() {
  delete [] datas;
  delete [] created;
}

void tag_cache_sim_t::init() {
  datas = new uint8_t[sets*ways*linesz]();
  created = new uint8_t[sets*ways]();
  tag_map = NULL;
  if(empty_block == NULL) empty_block = new uint8_t[linesz]();
}

uint64_t* tag_cache_sim_t::check_tag(uint64_t addr, size_t &row) {
  size_t idx = (addr >> idx_shift) & (sets-1);
  size_t tag = (addr >> idx_shift) | VALID;

  if(this->name == std::string("TM1$")){
    for (size_t i = 0; i< ways; i++){
      if (!created[idx*ways + i]){
        tags[idx*ways + i] = tag;
        created[idx*ways + i] = true;
        break;
      }
    }
  }

  for (size_t i = 0; i < ways; i++)
    if (tag == (tags[idx*ways + i] & ~(DIRTY|TAGFLAG))) {
      row = idx*ways + i;
      return &tags[idx*ways + i];
    }
  return NULL;
}

uint64_t* tag_cache_sim_t::victimize(uint64_t addr, size_t &row) {
  size_t idx = (addr >> idx_shift) & (sets-1);
  size_t way = ways;
  // check whether there is invalid or empty line first
  //for (size_t i = 0; i < ways; i++) {
  //  if((tags[idx*ways + i] & (VALID|TAGFLAG)) != (VALID|TAGFLAG)) {
  //    way = i;
  //    break;
  //  }
  //}
  if(way == ways) way = lfsr.next() % ways;
  row = idx*ways + way;
  return &tags[row];
}

void tag_cache_sim_t::refill(uint64_t addr, size_t row) {
  tags[row] = (addr >> idx_shift) | VALID;
  addr = tags[row] << idx_shift;
  memcpy(datas + row*linesz, sim->addr_to_tagmem(addr), linesz);
  if(memcmp(datas + row*linesz, empty_block, linesz)) {
    tags[row] |= TAGFLAG;
  }
}

void tag_cache_sim_t::writeback(size_t row) {
  if((tags[row] & (VALID|DIRTY)) == (VALID|DIRTY)) {
    uint64_t addr = tags[row] << idx_shift;
    if(wb_enforce || (tags[row] & TAGFLAG) || tg::is_top(addr)) {
      memcpy(sim->addr_to_tagmem(addr), datas + row*linesz, linesz);
      writebacks++;
    }
  }
  tags[row] = 0;
}

uint64_t tag_cache_sim_t::read(uint64_t addr, uint64_t &data, uint8_t fetch, uint8_t writeacc) {
  size_t row;
  uint64_t *tag = check_tag(addr, row);
  if(tag == NULL) {
    if(!fetch) return 0;
    tag = victimize(addr, row);
    writeback(row);
    refill(addr, row);
    writeacc? write_misses++:read_misses++;
  }
  if((*tag) & TAGFLAG) {        // read data
    data = *(uint64_t *)(datas + row*linesz + subrow(addr));
  } else {                      // cached but empty
    data = 0;
  }
  return (*tag) & ~DIRTY;
}

uint64_t tag_cache_sim_t::write(uint64_t addr, uint64_t data, uint64_t mask) {
  uint64_t rdata, wdata;
  size_t row;
  uint64_t *tag = check_tag(addr, row);
  bool tagFlag_chg = false;
  if(tag == NULL) {
    tag = victimize(addr, row);
    writeback(row);
    refill(addr, row);
    write_misses++;
  }
  rdata = *(uint64_t *)(datas + row*linesz + subrow(addr));
  wdata = (rdata & ~mask) | (data & mask); // get the udpate data
  if(rdata != wdata) {                     // update
    *(uint64_t *)(datas + row*linesz + subrow(addr)) = wdata;
    *tag |= DIRTY;
    if(!memcmp(datas + row*linesz, empty_block, linesz)) {
      tagFlag_chg = (*tag & TAGFLAG);
      *tag &= ~TAGFLAG;
    } else {
      tagFlag_chg = (*tag & TAGFLAG) == 0;
      *tag |= TAGFLAG;
    }
  }
  return tagFlag_chg ? *tag | DIRTY : *tag & ~DIRTY;
}

uint64_t tag_cache_sim_t::create(uint64_t addr, uint64_t data, uint64_t mask) {
  uint64_t rdata, wdata;
  size_t row;
  uint64_t *tag = check_tag(addr, row);
  bool tagFlag_chg = false;
  if(tag == NULL) {
    tag = victimize(addr, row);
    writeback(row);
    memcpy(datas + row*linesz, empty_block, linesz); // empty block
    *tag |= (addr >> idx_shift) | VALID | DIRTY;
    rdata = 0;
  } else {
    rdata = *(uint64_t *)(datas + row*linesz + subrow(addr));
    assert(rdata == 0);
  }
  wdata = (rdata & ~mask) | (data & mask); // get the udpate data
  if(rdata != wdata) {                     // update
    *(uint64_t *)(datas + row*linesz + subrow(addr)) = wdata;
    *tag |= DIRTY;
    if(!memcmp(datas + row*linesz, empty_block, linesz)) {
      *tag &= ~TAGFLAG;
    } else {
      tagFlag_chg = true;
      *tag |= TAGFLAG;
    }
  }
  return tagFlag_chg ? *tag | DIRTY : *tag & ~DIRTY;
}

// ----------------- separate tag table class ------------------- //

tag_cache_sim_t* sep_tag_cache_sim_t::construct(const char* config, const char* name, sim_t* sim, int level, size_t tagsz) {
  // sets:ways:blocksize:wb
  std::vector<std::string> args;
  std::string config_string = std::string(config);
  boost::split(args, config_string, boost::is_any_of(":"));
  assert(args.size() == 4);

  size_t sets = atoi(args[0].c_str());
  size_t ways = atoi(args[1].c_str());
  size_t linesz = atoi(args[2].c_str());
  size_t wb = atoi(args[3].c_str());

  tg::record_tc(level, tagsz, linesz);
  return new sep_tag_cache_sim_t(sets, ways, linesz, wb, name, sim, level);
}

uint64_t sep_tag_cache_sim_t::access(uint64_t addr, size_t bytes, bool store) {
  if(level == 0) addr &= ~((uint64_t)bytes-1);
  uint64_t tag_tag, tag_data = 0, tag_addr = tg::addr_conv(level, addr);
  uint64_t wmask = level == 0 ? tg::mask(level, addr, bytes) : tg::mask(level, addr, 0);
  uint64_t wdata = level == 0 ? read_mem(tag_addr) : (bytes ? wmask : 0); // mmu does not write tm bits
  bool map_bit = true;

  if(tag_map != NULL) map_bit = tag_map->access(tag_addr, 0, 0);
  if(map_bit) tag_tag = read(tag_addr, tag_data, 1, store);
  bool wen = (wdata & wmask) != (tag_data & wmask);
  if(store && wen) {
    if(map_bit || wb_enforce) tag_tag = write(tag_addr, wdata, wmask);
    else                      tag_tag = create(tag_addr, wdata, wmask);
    if((tag_tag & DIRTY) && tag_map != NULL)
      tag_map->access(tag_addr, (tag_tag & TAGFLAG) != 0, 1); // mmu does not write tm bits
  }

#ifdef TC_STAT_BEHAV
  store ? write_accesses++ : read_accesses++;
#else
  store && wen ? write_accesses++ : read_accesses++;
#endif

  if(level == 0 && !store) {
    assert((wdata & wmask) == (tag_data & wmask));
  }

  return store ? 0 : tag_data & wmask;
}

// ----------------- unified tag cache class -------------------- //

tag_cache_sim_t* uni_tag_cache_sim_t::construct(const char* config, const char* name, sim_t* sim, size_t tagsz) {
  // sets:ways:blocksize:wb
  std::vector<std::string> args;
  std::string config_string = std::string(config);
  boost::split(args, config_string, boost::is_any_of(":"));
  assert(args.size() == 4);

  size_t sets = atoi(args[0].c_str());
  size_t ways = atoi(args[1].c_str());
  size_t linesz = atoi(args[2].c_str());
  size_t wb = atoi(args[3].c_str());

  tg::record_tc(0, tagsz, linesz);
  tg::record_tc(1, 1, linesz);
  tg::record_tc(2, 1, linesz);
  return new uni_tag_cache_sim_t(sets, ways, linesz, wb, name, sim);
}

uint64_t uni_tag_cache_sim_t::access(uint64_t addr, size_t bytes, bool store) {
  uint64_t tt_tag = 0,  tt_data = 0,  tt_addr = tg::addr_conv(0, addr);
  uint64_t tm0_tag = 0, tm0_data = 0, tm0_addr = tg::addr_conv(1, tt_addr);
  uint64_t tm1_tag = 0, tm1_data = 0, tm1_addr = tg::addr_conv(2, tm0_addr);
  uint64_t tt_wmask = tg::mask(0, addr, bytes);
  uint64_t tt_wdata = read_mem(tt_addr);
  uint64_t tm0_wmask = tg::mask(1, tt_addr, 0);
  uint64_t tm1_wmask = tg::mask(2, tm0_addr, 0);
  bool wen = false;

  enum state_t {
    s_idle, s_tt_r, s_tm0_r, 
    s_tt_w, 
    s_tm1_f, s_tm0_f, s_tt_f,s_tm1_l, s_tm0_fl,s_tm0_cl, s_tt_fl,s_tt_cl,
    s_tm0_w, s_tm1_w
    };

  state_t state = s_tt_r;
  uint64_t rv = 0;

  do {
    switch(state) {
    case s_tt_r:                // read tag table
      tt_tag = read(tt_addr, tt_data, 0, store);
      if(tt_tag & VALID)        // hit
        state = store && (tt_wdata & tt_wmask) != (tt_data & tt_wmask) ? s_tm1_l : s_idle;
      else
        state = s_tm0_r;
      break; // s_tt_r is right.
    case s_tm0_r:               // read tag map 0
      tm0_tag = read(tm0_addr, tm0_data, 0, store);
      if(tm0_tag & VALID) {     // hit
        if(tm0_data & tm0_wmask)   // tagFlag = 1
          state = s_tt_f;  // tm0 hit, R/W to the corresponding tt.
        else
          state = store && (tt_wdata & tt_wmask) != 0 ? s_tm1_l : s_idle;
      } else  //tm0 miss
        state = s_tm1_f;
      break;
    case s_tm1_f: 
      tm1_tag = read(tm1_addr, tm1_data, 1, store);
      if (tm1_data & tm1_wmask)   // tagFlag = 1
        state = s_tm0_f ; // tagFlag in tm1 is 1, Fetch the corresponding cache line in tm0.
      else 
        state = store && (tt_wdata & tt_wmask) != 0 ? s_tm1_l : s_idle;
      break;
    case s_tm0_f :
      tm0_tag = read(tm0_addr, tm0_data, 1, store);
      if (tm0_data & tm0_wmask)     //tagFlag = 1;
        state = s_tt_f;
      else 
        state = store && (tt_wdata & tt_wmask) != 0 ? s_tm1_l : s_idle; 
      break;
    case s_tt_f :
      tt_tag = read(tt_addr, tt_data, 1, store);
      state = store && (tt_wdata & tt_wmask) != (tt_data & tt_wmask) ? s_tm1_l : s_idle;
      break;
    case s_tm1_l :
      tm1_tag = read(tm1_addr,tm1_data,1, store);
      if (tm1_data & tm1_wmask)   //tagFlag = 1
        state = s_tm0_fl;
      else 
        state = s_tm0_cl;
      break;
    case s_tm0_fl :
      tm0_tag = read(tm0_addr,tm0_data,1,store);
      if (tm0_data & tm0_wmask)  //tagFlag = 1
        state = s_tt_fl;
      else 
        state = s_tt_cl;
      break;
    case s_tm0_cl:
      tm0_tag = create(tm0_addr,0,0);
      state = s_tt_cl;
      break;
    case s_tt_fl :
      tt_tag = read(tt_addr,tt_data,1,store);
      state = s_tt_w;
      break;
    case s_tt_cl:
      tt_tag = create(tt_addr, 0,0);
      state = s_tt_w;
      break;
    case s_tt_w:
      //TODO invalid tt if it's empty
      tt_tag = write(tt_addr, tt_wdata, tt_wmask);
      state = (tt_tag & DIRTY) ? s_tm0_w : s_idle;
      wen = true;
      break;
    case s_tm0_w:
      //TODO invalid tm0 if it's empty
      tm0_tag = write(tm0_addr, (tt_tag & TAGFLAG) ? tm0_wmask : 0, tm0_wmask);
      state = (tm0_tag & DIRTY) ? s_tm1_w : s_idle;
      break;
    case s_tm1_w:
      tm1_tag = write(tm1_addr, (tm0_tag & TAGFLAG) ? tm1_wmask : 0, tm1_wmask);
      state = s_idle;
      break;
    default:
      assert(state == s_idle);
      // even idle is not reachable here
    }
  } while(state != s_idle);


#ifdef TC_STAT_BEHAV
    store ? write_accesses++ : read_accesses++;
#else
    store && wen ? write_accesses++ : read_accesses++;
#endif
    return rv;
}
