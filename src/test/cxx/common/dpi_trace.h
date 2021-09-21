// See LICENSE for license details.

#ifndef DPI_TRACE_BEHAV_H
#define DPI_TRACE_BEHAV_H

#include <svdpi.h>
#include <ostream>

#ifdef __cplusplus
extern "C" {
#endif

  extern svBit dpi_tc_send_packet (
				   const svBit ready,
				   svBit *valid,
				   svBitVecVal *addr,
				   svBitVecVal *id,
				   svBitVecVal *beat,
				   svBitVecVal *a_type,
				   svBitVecVal *tag
				   );
   
  extern svBit dpi_tc_send_packet_ack (
				   const svBit ready,
				   const svBit valid
				   );
   
  extern svBit dpi_tc_recv_packet (
				   const svBit valid,
				   const svBitVecVal *id,
				   const svBitVecVal *beat,
				   const svBitVecVal *g_type,
				   const svBitVecVal *tag
				   );

  extern svBit dpi_tc_init        (const int ncore);

#ifdef __cplusplus
}
#endif

#endif
