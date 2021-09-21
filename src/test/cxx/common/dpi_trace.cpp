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
std::ifstream data_trace, init_trace;
std::vector<trace> traces(max_trace);
std::set<int> trace_valid, trace_sent;
unsigned int trace_add_idx = 0, trace_send_idx = 0;
std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t> > trace_sending_queue;
std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t> > trace_pending_queue;
std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t> > recv_waiting_queue;
std::list<packet> send_pkt, recv_pkt;
std::list<trace> trace_list;

unsigned int ncore = 0;
uint64_t t_cycle_pre=0, t_cycle = 0, s_cycle = 0;

std::string to_string(const trace& t, int id) {
  return "[" + std::to_string(t.t_start) + ">" + std::to_string(t.t_delay) + "] (" + std::to_string(id) + ") "
    + std::to_string(t.addr) + (t.rw ? (" write " + std::to_string(t.tag)) : " read");
}

bool get_free_trace(unsigned int &idx) {
  if(trace_valid.size() >= max_trace) return false;
  while(trace_valid.count(idx)) idx = (idx + 1) %  max_trace;
  return true;
}

bool trace_read = false;
void read_traces() {
  //uint64_t init_trace_cnt = 0;
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
    //std::cout << "Record an init trace: " << to_string(trace_list.back(), 0) << std::endl;
  }
  
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
    //std::cout << "Record a data trace: " << to_string(trace_list.back(), 0) << std::endl;
  }
  std::cout << "Successfully read " << trace_list.size() << " traces in total." << std::endl;
  trace_read = true;
  init_trace.close();
  data_trace.close();
}

void fill_traces() {
  if(!trace_list.empty()) {
    while(get_free_trace(trace_add_idx)) {
      trace_valid.insert(trace_add_idx);
      traces[trace_add_idx] = trace_list.front();
      trace_list.pop_front();
      trace_sending_queue.push(traces[trace_add_idx].t_start);
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
  std::cout << t_cycle << "," << s_cycle << " Send a trace: " << to_string(traces[trace_send_idx], trace_send_idx) << std::endl;
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
			  svBitVecVal *tag
			  )
{
  send_trace();
  if(!send_pkt.empty()) {
    *valid = sv_1;
    addr[0] = send_pkt.front().addr;
    id[0]   = send_pkt.front().id;
    beat[0] = send_pkt.front().beat;
    a_type[0] = send_pkt.front().a_type;
    tag[0] = send_pkt.front().tag;
  } else
    *valid = sv_0;
  return sv_1;
}

svBit dpi_tc_send_packet_ack (const svBit ready, const svBit valid) {
  if(ready == sv_1 && valid == sv_1) {
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
  std::cout << t_cycle << "," << s_cycle << " Recv a trace: " << to_string(traces[id], id) << std::endl;
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

svBit dpi_tc_init(const int n) {
  ncore = n;
  trace_sending_queue.push(0); trace_sending_queue.pop();
  trace_pending_queue.push(0); trace_pending_queue.pop();
  recv_waiting_queue.push(0); recv_waiting_queue.pop();
  data_trace.open("trace-"+std::to_string(ncore)+".dat");
  init_trace.open("init-"+std::to_string(ncore)+".dat");
  read_traces();
  fill_traces();
}

