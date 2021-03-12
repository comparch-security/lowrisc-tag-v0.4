#include "bbl.h"
#include "pfc.h"

pfc_response pfc;


void get_pfc(pfc_response * ppfc)
{

uint64_t * pfcresp = ppfc->resp;
ppfc->instret = rdinstret();

#if(ENA_PFC) 
  asm volatile ("csrw 0x404, %0" :: "r"(pfc_fullmap));
  asm volatile ("csrw 0x403, %0" :: "r"(tilepfc_conf));
  asm volatile ("addi x0, x0, 0");
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[0]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[1]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[2]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[3]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[4]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[5]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[6]));

  
  
#if(L2Banks!=0)
  asm volatile ("csrw 0x404, %0" :: "r"(pfc_fullmap));
  asm volatile ("csrw 0x403, %0" :: "r"(l2b0pfc_conf));
  asm volatile ("addi x0, x0, 0");
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[7]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[8]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[9]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[10]));
#if(L2Banks>1)
  for(i=1;i<L2Banks;i++) {
    write_csr(0x404, pfc_fullmap);
    switch(i) {
      case 1:     asm volatile ("csrw 0x403, %0" :: "r"(l2b1pfc_conf));
      case 2:     asm volatile ("csrw 0x403, %0" :: "r"(l2b2pfc_conf));
      case 3:     asm volatile ("csrw 0x403, %0" :: "r"(l2b3pfc_conf));
      default:    asm volatile ("csrw 0x403, %0" :: "r"(l2b0pfc_conf));
    }
    __asm__("addi x0, x0, 0");
    pfcresp[7] =pfcresp[7] +read_csr(0x402);
    pfcresp[8] =pfcresp[8] +read_csr(0x402);
    pfcresp[9] =pfcresp[9] +read_csr(0x402);
    pfcresp[10]=pfcresp[10]+read_csr(0x402);
  }
#endif
#endif
 

 
#if(ADD_TC)
  asm volatile ("csrw 0x404, %0" :: "r"(pfc_fullmap));
  asm volatile ("csrw 0x403, %0" :: "r"(tcpfc_conf));
  __asm__("addi x0, x0, 0");
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[11]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[12]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[13]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[14]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[15]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[16]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[17]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[18]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[19]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[20]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[21]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[22]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[23]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[24]));
  asm volatile ("csrr %0, 0x402" : "=r"(pfcresp[25]));
#endif

#endif
}

void pfc_diff (pfc_response * result, pfc_response * decline)
{
    result->instret -= decline->instret;
    for (int i = 0 ; i < pfc_total_resp; i ++)
        result->resp[i] -= decline->resp[i];
}

void pfc_display(pfc_response * ppfc)
{

uint64_t * pfcresp = ppfc->resp;
uint64_t instret = ppfc -> instret;


#ifdef ENA_PFC

  printk("instret: %15lld\n\n",instret);

  printk("L1I_read,     L1I_readmiss\n");
  printk("%lld  %10lld\n\n", pfcresp[0], pfcresp[1]);
  printk("L1D_read,     L1D_readmiss,     L1D_write,     L1D_writemiss,     L1D_writeback\n");
  printk("%lld  %10lld  %10lld  %10lld  %10lld\n\n", pfcresp[2],pfcresp[3],pfcresp[4],pfcresp[5],pfcresp[6]);

#if(L2Banks!=0)
  printk("L2_read,      L2_readmiss,      L2_write,      L2_writeback\n");
  printk("%lld  %10lld  %10lld  %10lld\n\n", pfcresp[7],pfcresp[8],pfcresp[9],pfcresp[10]);
#endif

#if(ADD_TC)
  printk("TC_readTT,   TC_readTTmiss,    TC_writeTT,   TC_writeTTmiss,    TC_writeTTback\n");
  printk("%lld  %10lld  %10lld  %10lld  %10lld\n\n", pfcresp[11],pfcresp[12],pfcresp[13],pfcresp[14],pfcresp[15]);
  printk("TC_readTM0,  TC_readTM0miss,   TC_writeTM0,  TC_writeTM0miss,   TC_writeTM0back\n");
  printk("%lld  %10lld  %10lld  %10lld  %10lld\n\n", pfcresp[16],pfcresp[17],pfcresp[18],pfcresp[19],pfcresp[20]);
  printk("TC_readTM1,  TC_readTM1miss,   TC_writeTM1,  TC_writeTM1miss,   TC_writeTM1back\n");
  printk("%lld  %10lld  %10lld  %10lld  %10lld\n\n", pfcresp[21],pfcresp[22],pfcresp[23],pfcresp[24],pfcresp[25]);
#endif

#endif
}

