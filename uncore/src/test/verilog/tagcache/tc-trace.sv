// TC trace tests for parallel performance

`include "tagcache.consts.vh"

module tb;

   localparam TLAW    = `ROCKET_PADDR_WIDTH;
   localparam TLDW    = 64;
   localparam TLTW    = `TagBits;
   localparam TLBS    = 8;      // burst size
   localparam TLCIS   = 7;
   localparam TLMIS   = 2;

   reg              clk, reset, init;
   string 		    dscr;
   
   initial begin
      $value$plusargs("trace=%s", dscr);
   end

   logic            io_in_acquire_ready;
   reg              io_in_acquire_valid = 'b0;
   reg [TLAW-7:0]   io_in_acquire_bits_addr_block;
   reg [TLCIS-1:0]  io_in_acquire_bits_client_xact_id;
   reg [2:0]        io_in_acquire_bits_addr_beat;
   reg              io_in_acquire_bits_is_builtin_type;
   reg [2:0]        io_in_acquire_bits_a_type;
   reg [12:0]       io_in_acquire_bits_union;
   reg [TLDW-1:0]   io_in_acquire_bits_data;
   reg [TLTW-1:0]   io_in_acquire_bits_tag;
   reg              io_in_acquire_bits_client_id;
   reg              io_in_grant_ready = 'b1;
   logic            io_in_grant_valid;
   logic [2:0]      io_in_grant_bits_addr_beat;
   logic [TLCIS-1:0]io_in_grant_bits_client_xact_id;
   logic [TLMIS-1:0]io_in_grant_bits_manager_xact_id;
   logic            io_in_grant_bits_is_builtin_type;
   logic [3:0]      io_in_grant_bits_g_type;
   logic [TLDW-1:0] io_in_grant_bits_data;
   logic [TLTW-1:0] io_in_grant_bits_tag;
   logic            io_in_grant_bits_client_id;
   logic            io_in_finish_ready;
   reg              io_in_finish_valid = 'b0;
   reg [TLMIS-1:0]  io_in_finish_bits_manager_xact_id;
   reg              io_in_probe_ready;
   logic            io_in_probe_valid;
   logic [TLAW-7:0] io_in_probe_bits_addr_block;
   logic            io_in_probe_bits_p_type;
   logic            io_in_probe_bits_client_id;
   logic            io_in_release_ready;
   reg              io_in_release_valid = 'b0;
   reg [2:0]        io_in_release_bits_addr_beat;
   reg [TLAW-7:0]   io_in_release_bits_addr_block;
   reg [TLCIS-1:0]  io_in_release_bits_client_xact_id;
   reg              io_in_release_bits_voluntary;
   reg [1:0]        io_in_release_bits_r_type;
   reg [TLDW-1:0]   io_in_release_bits_data;
   reg [TLTW-1:0]   io_in_release_bits_tag;
   reg              io_in_release_bits_client_id;

   import "DPI-C" function bit
     dpi_tc_send_packet (
			 input bit 	   ready,
			 output bit 	   valid,
			 output bit [63:0] addr,
			 output bit [31:0] id,
			 output bit [2:0]  beat,
			 output bit [15:0] a_type,
			 output bit [7:0]  tag
			);

   import "DPI-C" function bit
     dpi_tc_send_packet_ack (
			 input bit 	   ready,
			 input bit 	   valid
			);
   
   import "DPI-C" function bit
     dpi_tc_recv_packet (
			 input bit 	  valid,
			 input bit [31:0] id,
			 input bit [2:0]  beat,
			 input bit [3:0]  g_type,
			 input bit [7:0]  tag
                         );
   import "DPI-C" function bit dpi_tc_init (input string dscr);

//   initial begin
//      #1800000000
//	$vcdpluson;
//      #700000000
//	$finish();
//   end

   initial begin
      reset = 'b1;
      dpi_tc_init(dscr);
      #77;
      reset = 0;
   end

   initial begin
      clk = 0;
      #80;
      forever #5 clk = !clk;
   end

   //initial begin
   //   #100000000
//	$finish();
   //end

   localparam MemAW   = `ROCKET_PADDR_WIDTH;
   localparam MemDW   = 64;

   nasti_channel
     #(
       .ID_WIDTH    ( 8       ),
       .ADDR_WIDTH  ( MemAW   ),
       .DATA_WIDTH  ( MemDW   ))
   mem_nasti();

   nasti_ram_behav
     #(
       .ID_WIDTH     ( 8       ),
       .ADDR_WIDTH   ( MemAW   ),
       .DATA_WIDTH   ( MemDW   ),
       .USER_WIDTH   ( 1       )
       )
   ram_behav
     (
      .clk           ( clk         ),
      .rstn          ( !reset      ),
      .nasti         ( mem_nasti   )
      );

   reg 					   recv_valid = 1;

   always @(negedge clk) begin
	dpi_tc_send_packet(
			   io_in_acquire_ready,io_in_acquire_valid,
			   io_in_acquire_bits_addr_block,
			   {io_in_acquire_bits_client_id,io_in_acquire_bits_client_xact_id},
			   io_in_acquire_bits_addr_beat,
			   {io_in_acquire_bits_union, io_in_acquire_bits_a_type},
			   io_in_acquire_bits_tag);
	io_in_acquire_bits_data = 0;
	io_in_acquire_bits_is_builtin_type = 'b1;

	recv_valid = dpi_tc_recv_packet(
					io_in_grant_valid,
					{io_in_grant_bits_client_id,io_in_grant_bits_client_xact_id},
					io_in_grant_bits_addr_beat,
					io_in_grant_bits_g_type,
					io_in_grant_bits_tag);

	if(!recv_valid)
	  $finish();

	#1 dpi_tc_send_packet_ack(io_in_acquire_ready, io_in_acquire_valid);
   end
	
   TagCacheTop DUT
     (
      .*,
      .io_out_aw_valid         ( mem_nasti.aw_valid                     ),
      .io_out_aw_ready         ( mem_nasti.aw_ready                     ),
      .io_out_aw_bits_id       ( mem_nasti.aw_id                        ),
      .io_out_aw_bits_addr     ( mem_nasti.aw_addr                      ),
      .io_out_aw_bits_len      ( mem_nasti.aw_len                       ),
      .io_out_aw_bits_size     ( mem_nasti.aw_size                      ),
      .io_out_aw_bits_burst    ( mem_nasti.aw_burst                     ),
      .io_out_aw_bits_lock     ( mem_nasti.aw_lock                      ),
      .io_out_aw_bits_cache    ( mem_nasti.aw_cache                     ),
      .io_out_aw_bits_prot     ( mem_nasti.aw_prot                      ),
      .io_out_aw_bits_qos      ( mem_nasti.aw_qos                       ),
      .io_out_aw_bits_region   ( mem_nasti.aw_region                    ),
      .io_out_aw_bits_user     ( mem_nasti.aw_user                      ),
      .io_out_w_valid          ( mem_nasti.w_valid                      ),
      .io_out_w_ready          ( mem_nasti.w_ready                      ),
      .io_out_w_bits_data      ( mem_nasti.w_data                       ),
      .io_out_w_bits_id        (                                        ),
      .io_out_w_bits_strb      ( mem_nasti.w_strb                       ),
      .io_out_w_bits_last      ( mem_nasti.w_last                       ),
      .io_out_w_bits_user      ( mem_nasti.w_user                       ),
      .io_out_b_valid          ( mem_nasti.b_valid                      ),
      .io_out_b_ready          ( mem_nasti.b_ready                      ),
      .io_out_b_bits_id        ( mem_nasti.b_id                         ),
      .io_out_b_bits_resp      ( mem_nasti.b_resp                       ),
      .io_out_b_bits_user      ( mem_nasti.b_user                       ),
      .io_out_ar_valid         ( mem_nasti.ar_valid                     ),
      .io_out_ar_ready         ( mem_nasti.ar_ready                     ),
      .io_out_ar_bits_id       ( mem_nasti.ar_id                        ),
      .io_out_ar_bits_addr     ( mem_nasti.ar_addr                      ),
      .io_out_ar_bits_len      ( mem_nasti.ar_len                       ),
      .io_out_ar_bits_size     ( mem_nasti.ar_size                      ),
      .io_out_ar_bits_burst    ( mem_nasti.ar_burst                     ),
      .io_out_ar_bits_lock     ( mem_nasti.ar_lock                      ),
      .io_out_ar_bits_cache    ( mem_nasti.ar_cache                     ),
      .io_out_ar_bits_prot     ( mem_nasti.ar_prot                      ),
      .io_out_ar_bits_qos      ( mem_nasti.ar_qos                       ),
      .io_out_ar_bits_region   ( mem_nasti.ar_region                    ),
      .io_out_ar_bits_user     ( mem_nasti.ar_user                      ),
      .io_out_r_valid          ( mem_nasti.r_valid                      ),
      .io_out_r_ready          ( mem_nasti.r_ready                      ),
      .io_out_r_bits_id        ( mem_nasti.r_id                         ),
      .io_out_r_bits_data      ( mem_nasti.r_data                       ),
      .io_out_r_bits_resp      ( mem_nasti.r_resp                       ),
      .io_out_r_bits_last      ( mem_nasti.r_last                       ),
      .io_out_r_bits_user      ( mem_nasti.r_user                       ),
      .io_getpfc               ( 1'b0                                   )
      );
   
endmodule // tb
