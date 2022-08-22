#include "dpi_trace.h"
#include <cstdint>
#include <cstring>
#include <cassert>
#include <string>
#include <fstream>
#include <iostream>
#include <queue>
#include <map>
#include <set>
#include <vector>
#include <list>
#include <boost/format.hpp>

const int max_trace = 1 << 7;

struct trace {
  uint64_t t_start;
  uint64_t t_delay;
  uint64_t addr;
  bool rw;
  uint64_t tag;
};

struct packet {
  uint64_t addr;
  uint32_t id;
  uint16_t a_type;
  uint8_t beat;
  uint8_t tag;
};

int state = 0;
uint64_t trace_send_count = 0;
uint64_t trace_data_end, trace_warm_end, trace_init_end;
std::ifstream data_trace, warm_trace, init_trace;
std::vector<trace> traces(max_trace);
std::set<int> trace_valid, trace_sent;
unsigned int trace_add_idx = 0, trace_send_idx = 0;
std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t> > trace_sending_queue;
std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t> > trace_pending_queue;
std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t> > recv_waiting_queue;
std::list<packet> send_pkt, recv_pkt;
std::list<trace> trace_list;
uint64_t data_files, warm_files, init_files;
std::string data_prefix, warm_prefix, init_prefix;
uint64_t data_trace_cnt, warm_trace_cnt, init_trace_cnt;
bool data_trace_exausted, warm_trace_exausted, init_trace_exausted;
uint64_t trace_cnt;
const uint64_t trace_read_bound = 512lu * 1024lu;

uint64_t t_cycle_pre=0, t_cycle = 0, s_cycle = 0;

std::string to_string(const trace& t, int id) {
  return "[" + std::to_string(t.t_start) + ">" + std::to_string(t.t_delay) + "] (" + std::to_string(id) + ") "
    + (boost::format("%016x") % t.addr).str() //std::to_string(t.addr)
    + (t.rw ? (" write "
               + (boost::format("%8x") % t.tag).str() //std::to_string(t.tag)
               ) : " read");
}

bool get_free_trace(unsigned int &idx) {
  if(trace_valid.size() >= max_trace) return false;
  while(trace_valid.count(idx)) idx = (idx + 1) %  max_trace;
  return true;
}

bool trace_read = false;
void read_traces() {
  //uint64_t init_trace_cnt = 0;
  uint64_t traces = 0;
  if(trace_read) return;
  while(!init_trace.eof() /*&& init_trace_cnt++ < 10000 */) {
    char * cstr = new char[256];
    init_trace.getline(cstr, 256);
    if(cstr[0] == 0) break;
    trace_list.push_back(trace{0,0,0,true,0});
    char *token;
    token = strtok(cstr, ",");
    trace_list.back().addr = std::stoll(token, NULL, 16);
    token = strtok(NULL, ",");
    trace_list.back().tag = std::stoll(token, NULL, 16);
    delete[] cstr;
    traces ++;
    //std::cout << "Record an init trace: " << to_string(trace_list.back(), 0) << std::endl;
  }
  init_trace.close();
  trace_init_end = traces;
  
  while(!warm_trace.eof()) {
    char * cstr = new char[256];
    warm_trace.getline(cstr, 256);
    if(cstr[0] == 0) break;
    trace_list.push_back(trace{0,0,0,false,0});
    char *token;
    token = strtok(cstr, ",");
    token = strtok(NULL, ",");
    token = strtok(NULL, ",");
    trace_list.back().addr = std::stoll(token, NULL, 16);
    token = strtok(NULL, ",");
    trace_list.back().rw = std::stoi(token);
    if(trace_list.back().rw) {
      token = strtok(NULL, ",");
      trace_list.back().tag = std::stoll(token, NULL, 16);
    }
    delete[] cstr;
    traces ++;
    //std::cout << "Record a warm trace: " << to_string(trace_list.back(), 0) << std::endl;
  }
  warm_trace.close();
  trace_warm_end = traces;

  while(!data_trace.eof()) {
    char * cstr = new char[256];
    data_trace.getline(cstr, 256);
    if(cstr[0] == 0) break;
    trace_list.push_back(trace{0,0,0,false,0});
    char *token;
    token = strtok(cstr, ",");
    t_cycle_pre += std::stoll(token, NULL,16);
    trace_list.back().t_start = t_cycle_pre;
    token = strtok(NULL, ",");
    trace_list.back().t_delay = t_cycle_pre + std::stoll(token, NULL,16);
    token = strtok(NULL, ",");
    trace_list.back().addr = std::stoll(token, NULL, 16);
    token = strtok(NULL, ",");
    trace_list.back().rw = std::stoi(token);
    if(trace_list.back().rw) {
      token = strtok(NULL, ",");
      trace_list.back().tag = std::stoll(token, NULL, 16);
    }
    delete[] cstr;
    traces ++;
    //std::cout << "Record a data trace: " << to_string(trace_list.back(), 0) << std::endl;
  }
  trace_data_end = traces;
  // std::cout << "Successfully read " << trace_list.size() << " traces in total." << std::endl;
  std::cout << "trace_init_end " << trace_init_end << " trace_warm_end " << trace_warm_end << " trace_data_end " << trace_data_end << std::endl;
  trace_read = true;
  data_trace.close();
}

void read_traces_filewise();
void fill_traces() {
  if(!trace_list.empty()) {
    while(get_free_trace(trace_add_idx)) {
      trace_valid.insert(trace_add_idx);
      traces[trace_add_idx] = trace_list.front();
      trace_list.pop_front();
      trace_sending_queue.push(traces[trace_add_idx].t_start);
      if(trace_list.empty()) 
        read_traces_filewise();
      //std::cout << "Load a trace: " << to_string(traces[trace_add_idx], trace_add_idx) << std::endl;
    }
  }
}

bool get_send_trace(unsigned int &idx) {
  if(trace_sending_queue.empty()) return false;
  if(trace_sending_queue.size() > 0 && t_cycle < trace_sending_queue.top()) return false;
  while(!trace_valid.count(idx) || traces[idx].t_start > t_cycle || trace_sent.count(idx)) idx = (idx + 1) %  max_trace;
  return true;
}

void send_trace() {
  fill_traces();
  if(send_pkt.size() >= 10) return;
  //if(trace_pending_queue.size() > 0 && t_cycle < trace_pending_queue.top()) return;
  if(!get_send_trace(trace_send_idx)) return;
  trace_sent.insert(trace_send_idx);
  trace_sending_queue.pop();
  trace_pending_queue.push(traces[trace_send_idx].t_delay);
  // std::cout << t_cycle << "," << s_cycle << " Send a trace: " << to_string(traces[trace_send_idx], trace_send_idx) << std::endl;
  if(traces[trace_send_idx].rw) {
    send_pkt.push_back(packet{traces[trace_send_idx].addr >> 6, trace_send_idx, 0xfffb, 0, (uint8_t)((traces[trace_send_idx].tag >> 0)  & 0x3)});
    send_pkt.push_back(packet{traces[trace_send_idx].addr >> 6, trace_send_idx, 0xfffb, 1, (uint8_t)((traces[trace_send_idx].tag >> 2)  & 0x3)});
    send_pkt.push_back(packet{traces[trace_send_idx].addr >> 6, trace_send_idx, 0xfffb, 2, (uint8_t)((traces[trace_send_idx].tag >> 4)  & 0x3)});
    send_pkt.push_back(packet{traces[trace_send_idx].addr >> 6, trace_send_idx, 0xfffb, 3, (uint8_t)((traces[trace_send_idx].tag >> 6)  & 0x3)});
    send_pkt.push_back(packet{traces[trace_send_idx].addr >> 6, trace_send_idx, 0xfffb, 4, (uint8_t)((traces[trace_send_idx].tag >> 8)  & 0x3)});
    send_pkt.push_back(packet{traces[trace_send_idx].addr >> 6, trace_send_idx, 0xfffb, 5, (uint8_t)((traces[trace_send_idx].tag >> 10) & 0x3)});
    send_pkt.push_back(packet{traces[trace_send_idx].addr >> 6, trace_send_idx, 0xfffb, 6, (uint8_t)((traces[trace_send_idx].tag >> 12) & 0x3)});
    send_pkt.push_back(packet{traces[trace_send_idx].addr >> 6, trace_send_idx, 0xfffb, 7, (uint8_t)((traces[trace_send_idx].tag >> 14) & 0x3)});
  } else {
    send_pkt.push_back(packet{traces[trace_send_idx].addr >> 6, trace_send_idx, 0x0e09, 0, 0});
  }
}

svBit dpi_tc_send_packet (const svBit ready, svBit *valid,
			  svBitVecVal *addr,
			  svBitVecVal *id,
			  svBitVecVal *beat,
			  svBitVecVal *a_type,
			  svBitVecVal *tag,
        svBit       *getpfc
			  )
{
  send_trace();
  *getpfc = sv_0;
  if(!send_pkt.empty()) {
    *valid = sv_1;
    addr[0] = send_pkt.front().addr;
    id[0]   = send_pkt.front().id;
    beat[0] = send_pkt.front().beat;
    a_type[0] = send_pkt.front().a_type;
    tag[0] = send_pkt.front().tag;
    if(trace_send_count + 1 == trace_init_end ||
       trace_send_count + 1 == trace_warm_end ||
       trace_send_count + 1 == trace_data_end) {
      if(ready == sv_1 &&
         ((send_pkt.front().a_type == 0xfffb && send_pkt.front().beat == 7) ||
          (send_pkt.front().a_type == 0x0e09 && send_pkt.front().beat == 0)))
        *getpfc = sv_1;
    }
  } else
    *valid = sv_0;

  return sv_1;
}

svBit dpi_tc_send_packet_ack (const svBit ready, const svBit valid) {
  if(ready == sv_1 && valid == sv_1) {
    if((send_pkt.front().a_type == 0xfffb && send_pkt.front().beat == 7) ||
       (send_pkt.front().a_type == 0x0e09 && send_pkt.front().beat == 0))
      trace_send_count ++;
    send_pkt.pop_front();
  }
  return sv_1;
}

void recv_trace() {
  if(recv_pkt.empty()) return;
  if(recv_pkt.front().a_type == 0x5 && recv_pkt.size() < 8) return;
  uint32_t id = recv_pkt.front().id;
  assert(trace_sent.count(id));
  recv_waiting_queue.push(traces[id].t_delay);
  traces[id].t_delay = t_cycle;
  if(recv_pkt.front().a_type == 0x5) { // read block
    traces[id].tag = 0;
    for(int i=0; i<8; i++) {
      traces[id].tag <<= 2;
      traces[id].tag |= (recv_pkt.front().tag & 0x3);
      assert(recv_pkt.front().beat == i);
      assert(recv_pkt.front().id == id);
      recv_pkt.pop_front();
    }
  } else {
    recv_pkt.pop_front();
  }
  trace_sent.erase(id);
  trace_valid.erase(id);
  //std::cout << t_cycle << "," << s_cycle << " Recv a trace: " << to_string(traces[id], id) << std::endl;
}

svBit dpi_tc_recv_packet (const svBit valid,
			  const svBitVecVal *id,
			  const svBitVecVal *beat,
			  const svBitVecVal *g_type,
			  const svBitVecVal *tag
			  )
{
  if(valid == sv_1) {
    recv_pkt.push_back(packet{0, SV_GET_UNSIGNED_BITS(id[0], 16), SV_GET_UNSIGNED_BITS(g_type[0], 4), (uint8_t)(SV_GET_UNSIGNED_BITS(beat[0], 8)),
	  SV_GET_UNSIGNED_BITS(tag[0], 8)});
  }
  recv_trace();

  if(!recv_waiting_queue.empty() && recv_waiting_queue.top() == trace_pending_queue.top()) {
    recv_waiting_queue.pop();
    trace_pending_queue.pop();
  }

  if(((trace_pending_queue.empty() || t_cycle < trace_pending_queue.top()) && (t_cycle !=0 || trace_sending_queue.top() > 0)))
    t_cycle++;

  if(t_cycle != 0) s_cycle++;

  if(recv_pkt.empty() && trace_valid.empty())
    return sv_0;
  else
    return sv_1;
}

void data_trace_begin(std::string dscr) {
  data_files = 0;
  data_trace_exausted = false;
  data_prefix = std::string("trace-"+dscr+"-");
  data_trace.open(data_prefix+std::to_string(data_files)+".dat");
}

void warm_trace_begin(std::string dscr) {
  warm_files = 0;
  warm_trace_exausted = false;
  warm_prefix = std::string("warm-"+dscr+"-");
  warm_trace.open(warm_prefix+std::to_string(warm_files)+".dat");
}

void init_trace_begin(std::string dscr) {
  init_files = 0;
  init_trace_exausted = false;
  init_prefix = std::string("init-"+dscr+"-");
  init_trace.open(init_prefix+std::to_string(init_files)+".dat");
}

void read_init_trace() {
  if(init_trace_exausted) return ;
  init_trace_cnt = 0;
  while(!init_trace.eof()) {
    char * cstr = new char[256];
    init_trace.getline(cstr, 256);
    if(cstr[0] == 0) break;
    trace_list.push_back(trace{0,0,0,true,0});
    char *token;
    token = strtok(cstr, ",");
    trace_list.back().addr = std::stoll(token, NULL, 16);
    token = strtok(NULL, ",");
    trace_list.back().tag = std::stoll(token, NULL, 16);
    delete[] cstr;
    trace_cnt ++;
    //std::cout << "Record an init trace: " << to_string(trace_list.back(), 0) << std::endl;
  }
  init_trace.close();
  init_files ++ ;
  init_trace.open(init_prefix+std::to_string(init_files)+".dat");
  if (!init_trace.is_open()) {
    init_trace_exausted = true;
    init_trace_cnt = trace_cnt;
    trace_init_end = trace_cnt;
  }
}

void read_warm_trace() {
  if(warm_trace_exausted) return; 
  warm_trace_cnt = 0;
  while(!warm_trace.eof()) {
    char * cstr = new char[256];
    warm_trace.getline(cstr, 256);
    if(cstr[0] == 0) break;
    trace_list.push_back(trace{0,0,0,false,0});
    char *token;
    token = strtok(cstr, ",");
    token = strtok(NULL, ",");
    token = strtok(NULL, ",");
    trace_list.back().addr = std::stoll(token, NULL, 16);
    token = strtok(NULL, ",");
    trace_list.back().rw = std::stoi(token);
    if(trace_list.back().rw) {
      token = strtok(NULL, ",");
      trace_list.back().tag = std::stoll(token, NULL, 16);
    }
    delete[] cstr;
    trace_cnt ++;
    //std::cout << "Record a warm trace: " << to_string(trace_list.back(), 0) << std::endl;
  }
  warm_trace.close();
  warm_files ++ ;
  warm_trace.open(warm_prefix+std::to_string(warm_files)+".dat");
  if (!warm_trace.is_open()) {
    warm_trace_exausted = true;
    warm_trace_cnt = trace_cnt;
    trace_warm_end = trace_cnt;
  }
}

void read_data_trace() {
  if(data_trace_exausted) return;
  data_trace_cnt = 0;
  while(!data_trace.eof()) {
    char * cstr = new char[256];
    data_trace.getline(cstr, 256);
    if(cstr[0] == 0) break;
    trace_list.push_back(trace{0,0,0,false,0});
    char *token;
    token = strtok(cstr, ",");
    t_cycle_pre += std::stoll(token, NULL,16);
    trace_list.back().t_start = t_cycle_pre;
    token = strtok(NULL, ",");
    trace_list.back().t_delay = t_cycle_pre + std::stoll(token, NULL,16);
    token = strtok(NULL, ",");
    trace_list.back().addr = std::stoll(token, NULL, 16);
    token = strtok(NULL, ",");
    trace_list.back().rw = std::stoi(token);
    if(trace_list.back().rw) {
      token = strtok(NULL, ",");
      trace_list.back().tag = std::stoll(token, NULL, 16);
    }
    delete[] cstr;
    trace_cnt ++;
    //std::cout << "Record a data trace: " << to_string(trace_list.back(), 0) << std::endl;
  }
  data_trace.close();
  data_files ++;
  data_trace.open(data_prefix+std::to_string(data_files)+".dat");
  if (!data_trace.is_open()) {
    data_trace_exausted = true;
    data_trace_cnt = trace_cnt;
    trace_data_end = trace_cnt;
  }
}

void read_traces_filewise() {
  bool initial_emptiness = trace_list.empty();
  if (!init_trace_exausted) {
    read_init_trace();
  }
  else if (!warm_trace_exausted) {
    read_warm_trace();
  }
  else if (!data_trace_exausted) {
    read_data_trace();
  }
  else {
    // All trace read. Do nothing.
    return ;
  }
  // skip empty trace type.
  if ((initial_emptiness == true) && trace_list.empty()) {
    if (init_trace_exausted && warm_trace_exausted 
          && data_trace_exausted)
      return ;
    else {
      read_traces_filewise();
    }
  }
  return;
}
 

svBit dpi_tc_init(const char * dscr) {
  trace_sending_queue.push(0); trace_sending_queue.pop();
  trace_pending_queue.push(0); trace_pending_queue.pop();
  recv_waiting_queue.push(0); recv_waiting_queue.pop();
  data_trace_begin(std::string(dscr));
  warm_trace_begin(std::string(dscr));
  init_trace_begin(std::string(dscr));
  read_traces_filewise();
  fill_traces();
  return 0;
}

svBit dpi_tc_finish() {
  std::cout << t_cycle << "," << s_cycle << std::endl;
  return 0;
}

