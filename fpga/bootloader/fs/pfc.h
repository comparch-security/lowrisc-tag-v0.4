#include <stdio.h>
#include "encoding.h"

#define ENA_PFC 1
#define L2Banks 1
#define ADD_TC 1

#define pfc_fullmap   0xffffffffffffffff
#define tilepfc_conf  0x0000000000000001
#define l2b0pfc_conf  0x0010000000000001  //l2 bank0 pfc
#define l2b1pfc_conf  0x0010000000000201  //l2 bank1 pfc
#define l2b2pfc_conf  0x0010000000000401  //l2 bank2 pfc
#define l2b3pfc_conf  0x0010000000000601  //l2 bank3 pfc
#define tcpfc_conf    0x0020000000000001

typedef struct {
#define pfc_total_resp 38  //Tile + L2Banks + TC=7+4+27
    uint64_t resp[pfc_total_resp]; 
    uint64_t instret;
    uint64_t cycles;
} pfc_response;

extern pfc_response pfc[3];

void get_pfc(pfc_response * ppfc);
void pfc_diff (pfc_response *start, pfc_response *end, pfc_response *result);
void pfc_display(pfc_response * ppfc);
void pfc_log(int code);
