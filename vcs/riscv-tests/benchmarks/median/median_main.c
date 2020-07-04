// See LICENSE for license details.

//**************************************************************************
// Median filter bencmark
//--------------------------------------------------------------------------
//
// This benchmark performs a 1D three element median filter. The
// input data (and reference data) should be generated using the
// median_gendata.pl perl script and dumped to a file named
// dataset1.h You should not change anything except the
// HOST_DEBUG and PREALLOCATE macros for your timing run.

#include "util.h"

#include "median.h"

//--------------------------------------------------------------------------
// Input/Reference Data

#include "dataset1.h"

//--------------------------------------------------------------------------
// Main

int main( int argc, char* argv[] )
{
  int results_data[DATA_SIZE];

  // Output the input array
  printArray( "input",  DATA_SIZE, input_data  );
  printArray( "verify", DATA_SIZE, verify_data );

#if PREALLOCATE
  // If needed we preallocate everything in the caches
  median( DATA_SIZE, input_data, results_data );
#endif

  // Do the filter
  setStats(1);
  median( DATA_SIZE, input_data, results_data );
  setStats(0);

  // Print out the results
  printArray( "results", DATA_SIZE, results_data );

// Check the results

//PFC
// read Tile0PFC
// 1:clear pfcc:pfc_config
__asm__("csrrw x0, 0x403, x0");
// 2:set pfcm:pfc_bitmap
__asm__("not x5, x0");
__asm__("csrrw x0, 0x404, x5");
// 3:set pfcc:pfc_config tile0
__asm__("addi x5, x0, 1");  //config pfcc.trigger=1
__asm__("csrrw x0, 0x403, x5");  //write pfcc
// 4:some nop wait pfcManager resp should use while(!pfcc.empty)
__asm__("addi x0, x0, 0");
__asm__("addi x0, x0, 0");

// read L2Bank0PFC
// 1:clear pfcc:pfc_config
__asm__("csrrw x0, 0x403, x0");
// 2:set pfcm:pfc_bitmap
__asm__("not x5, x0");
__asm__("csrrw x0, 0x404, x5");
// 3:set pfcc:pfc_config tile0
__asm__("addi x5, x0, 0x01");  
__asm__("slli x5, x5, 52");         //config pfcc.pfcMtype=1 means L2
__asm__("addi x5, x5, 0x01");       //config pfcc.trigger=1
__asm__("csrrw x0, 0x403, x5");     //write pfcc
// 4:some nop wait pfcManager resp should use while(!pfcc.empty)
__asm__("addi x0, x0, 0");
__asm__("addi x0, x0, 0");


// read L2Bank1PFC
// 1:clear pfcc:pfc_config
__asm__("csrrw x0, 0x403, x0");
// 2:set pfcm:pfc_bitmap
__asm__("not x5, x0");
__asm__("csrrw x0, 0x404, x5");
// 3:set pfcc:pfc_config tile0
__asm__("addi x5, x0, 0x01");   
__asm__("slli x5, x5, 52");           //config pfcc.pfcMtype=1 means L2
__asm__("addi x6, x0, 0x01");         //config pfcc.pfcMID=1
__asm__("slli x6, x6, 10");           //config pfcc.pfcMID=1
__asm__("add  x5, x5, x6");           //L2Bank1PFC = pfcc.pfcMtype=1 + pfcc.pfcMID=1
__asm__("addi x5, x5, 0x01");         //config pfcc.trigger=1
__asm__("csrrw x0, 0x403, x5");       //write pfcc
// 4:some nop wait pfcManager resp should use while(!pfcc.empty)
__asm__("addi x0, x0, 0");
__asm__("addi x0, x0, 0");

// read L2Bank2PFC
// 1:clear pfcc:pfc_config
__asm__("csrrw x0, 0x403, x0");
// 2:set pfcm:pfc_bitmap
__asm__("not x5, x0");
__asm__("csrrw x0, 0x404, x5");
// 3:set pfcc:pfc_config tile0
__asm__("addi x5, x0, 0x01");   
__asm__("slli x5, x5, 52");           //config pfcc.pfcMtype=1 means L2
__asm__("addi x6, x0, 0x02");         //config pfcc.pfcMID=2
__asm__("slli x6, x6, 10");           //config pfcc.pfcMID=2
__asm__("add  x5, x5, x6");           //L2Bank2PFC = pfcc.pfcMtype=1 + pfcc.pfcMID=2
__asm__("addi x5, x5, 0x01");         //config pfcc.trigger=1
__asm__("csrrw x0, 0x403, x5");       //write pfcc
// 4:some nop wait pfcManager resp should use while(!pfcc.empty)
__asm__("addi x0, x0, 0");
__asm__("addi x0, x0, 0");

// read L2Bank3PFC
// 1:clear pfcc:pfc_config
__asm__("csrrw x0, 0x403, x0");
// 2:set pfcm:pfc_bitmap
__asm__("not x5, x0");
__asm__("csrrw x0, 0x404, x5");
// 3:set pfcc:pfc_config tile0
__asm__("addi x5, x0, 0x01");   
__asm__("slli x5, x5, 52");           //config pfcc.pfcMtype=1 means L2
__asm__("addi x6, x0, 0x03");         //config pfcc.pfcMID=3
__asm__("slli x6, x6, 10");           //config pfcc.pfcMID=3
__asm__("add  x5, x5, x6");           //L2Bank3PFC = pfcc.pfcMtype=1 + pfcc.pfcMID=3
__asm__("addi x5, x5, 0x01");         //config pfcc.trigger=1
__asm__("csrrw x0, 0x403, x5");       //write pfcc
// some nop wait pfcManager resp should use while(!pfcc.empty)
__asm__("addi x0, x0, 0");
__asm__("addi x0, x0, 0");

// read TCPFC
// 1:clear pfcc:pfc_config
__asm__("csrrw x0, 0x403, x0");
// 2:set pfcm:pfc_bitmap
__asm__("not x5, x0");
__asm__("csrrw x0, 0x404, x5");
// 3:set pfcc:pfc_config tile0
__asm__("addi x5, x0, 0x02");   //
__asm__("slli x5, x5, 52");          //config pfcMtype=2 means TC
__asm__("addi x5, x5, 0x01");        //config pfcc.trigger
__asm__("csrrw x0, 0x403, x5");      //write pfcc
// 4:some nop wait pfcManager resp should use while(!pfcc.empty)
__asm__("addi x0, x0, 0");
__asm__("addi x0, x0, 0");

  return verify( DATA_SIZE, results_data, verify_data );
}
