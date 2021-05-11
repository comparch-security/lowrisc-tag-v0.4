// See LICENSE for license details.

`include "consts.vh"
`include "config.vh"

module tb;

   logic clk, rst;
   wire [7:0] led;

   chip_top
   DUT
     (
      .*,
      .clk_p        ( clk       ),
      .clk_n        ( !clk      ),
`ifdef FPGA_FULL
 `ifdef NEXYS4_COMMON
      .rst_top      ( !rst      )         // NEXYS4's cpu_reset is active low
 `else
      .rst_top      ( rst       )
 `endif
`else
      .rst_top      ( rst       )
`endif
      );

   initial begin
      rst = 1;
      #130;
      rst = 0;
   end

   initial begin
      clk = 0;
  `ifdef KC705
      forever clk = #2.5 !clk;
  `else
      forever clk = #5 !clk;
  `endif
   end // initial begin

`ifdef ADD_PHY_DDR
 `ifdef KC705

   // DDRAM3
   wire [63:0]  ddr_dq;
   wire [7:0]   ddr_dqs_n;
   wire [7:0]   ddr_dqs_p;
   logic [13:0] ddr_addr;
   logic [2:0]  ddr_ba;
   logic        ddr_ras_n;
   logic        ddr_cas_n;
   logic        ddr_we_n;
   logic        ddr_reset_n;
   logic        ddr_ck_p;
   logic        ddr_ck_n;
   logic        ddr_cke;
   logic        ddr_cs_n;
   wire [7:0]   ddr_dm;
   logic        ddr_odt;

   // behavioural DDR3 RAM
   genvar       i;
   generate
      for (i = 0; i < 8; i = i + 1) begin: gen_mem
         ddr3_model u_comp_ddr3
               (
                .rst_n   ( ddr_reset_n     ),
                .ck      ( ddr_ck_p        ),
                .ck_n    ( ddr_ck_n        ),
                .cke     ( ddr_cke         ),
                .cs_n    ( ddr_cs_n        ),
                .ras_n   ( ddr_ras_n       ),
                .cas_n   ( ddr_cas_n       ),
                .we_n    ( ddr_we_n        ),
                .dm_tdqs ( ddr_dm[i]       ),
                .ba      ( ddr_ba          ),
                .addr    ( ddr_addr        ),
                .dq      ( ddr_dq[8*i +:8] ),
                .dqs     ( ddr_dqs_p[i]    ),
                .dqs_n   ( ddr_dqs_n[i]    ),
                .tdqs_n  (                 ),
                .odt     ( ddr_odt         )
                );
      end
   endgenerate

 `elsif GENESYS2

   // DDRAM3
   wire [31:0]  ddr_dq;
   wire [3:0]   ddr_dqs_n;
   wire [3:0]   ddr_dqs_p;
   logic [14:0] ddr_addr;
   logic [2:0]  ddr_ba;
   logic        ddr_ras_n;
   logic        ddr_cas_n;
   logic        ddr_we_n;
   logic        ddr_reset_n;
   logic        ddr_ck_p;
   logic        ddr_ck_n;
   logic        ddr_cke;
   logic        ddr_cs_n;
   wire [3:0]   ddr_dm;
   logic        ddr_odt;

   // behavioural DDR3 RAM
   genvar       i;
   generate
      for (i = 0; i < 4; i = i + 1) begin: gen_mem
         ddr3_model u_comp_ddr3
               (
                .rst_n   ( ddr_reset_n     ),
                .ck      ( ddr_ck_p        ),
                .ck_n    ( ddr_ck_n        ),
                .cke     ( ddr_cke         ),
                .cs_n    ( ddr_cs_n        ),
                .ras_n   ( ddr_ras_n       ),
                .cas_n   ( ddr_cas_n       ),
                .we_n    ( ddr_we_n        ),
                .dm_tdqs ( ddr_dm[i]       ),
                .ba      ( ddr_ba          ),
                .addr    ( ddr_addr        ),
                .dq      ( ddr_dq[8*i +:8] ),
                .dqs     ( ddr_dqs_p[i]    ),
                .dqs_n   ( ddr_dqs_n[i]    ),
                .tdqs_n  (                 ),
                .odt     ( ddr_odt         )
                );
      end
   endgenerate

 `elsif NEXYS4_VIDEO

   // DDRAM3
   wire [15:0]  ddr_dq;
   wire [1:0]   ddr_dqs_n;
   wire [1:0]   ddr_dqs_p;
   logic [14:0] ddr_addr;
   logic [2:0]  ddr_ba;
   logic        ddr_ras_n;
   logic        ddr_cas_n;
   logic        ddr_we_n;
   logic        ddr_reset_n;
   logic        ddr_ck_p;
   logic        ddr_ck_n;
   logic        ddr_cke;
   logic        ddr_cs_n;
   wire [1:0]   ddr_dm;
   logic        ddr_odt;
   
   // behavioural DDR3 RAM
   genvar       i;
   generate
      for (i = 0; i < 2; i = i + 1) begin: gen_mem
         ddr3_model u_comp_ddr3
               (
                .rst_n   ( ddr_reset_n     ),
                .ck      ( ddr_ck_p        ),
                .ck_n    ( ddr_ck_n        ),
                .cke     ( ddr_cke         ),
                .cs_n    ( ddr_cs_n        ),
                .ras_n   ( ddr_ras_n       ),
                .cas_n   ( ddr_cas_n       ),
                .we_n    ( ddr_we_n        ),
                .dm_tdqs ( ddr_dm[i]       ),
                .ba      ( ddr_ba          ),
                .addr    ( ddr_addr        ),
                .dq      ( ddr_dq[8*i +:8] ),
                .dqs     ( ddr_dqs_p[i]    ),
                .dqs_n   ( ddr_dqs_n[i]    ),
                .tdqs_n  (                 ),
                .odt     ( ddr_odt         )
                );
      end
   endgenerate   

 `elsif NEXYS4

   wire [15:0]  ddr_dq;
   wire [1:0]   ddr_dqs_n;
   wire [1:0]   ddr_dqs_p;
   logic [12:0] ddr_addr;
   logic [2:0]  ddr_ba;
   logic        ddr_ras_n;
   logic        ddr_cas_n;
   logic        ddr_we_n;
   logic        ddr_ck_p;
   logic        ddr_ck_n;
   logic        ddr_cke;
   logic        ddr_cs_n;
   wire [1:0]   ddr_dm;
   logic        ddr_odt;

   // behavioural DDR2 RAM
   ddr2_model u_comp_ddr2
     (
      .ck      ( ddr_ck_p        ),
      .ck_n    ( ddr_ck_n        ),
      .cke     ( ddr_cke         ),
      .cs_n    ( ddr_cs_n        ),
      .ras_n   ( ddr_ras_n       ),
      .cas_n   ( ddr_cas_n       ),
      .we_n    ( ddr_we_n        ),
      .dm_rdqs ( ddr_dm          ),
      .ba      ( ddr_ba          ),
      .addr    ( ddr_addr        ),
      .dq      ( ddr_dq          ),
      .dqs     ( ddr_dqs_p       ),
      .dqs_n   ( ddr_dqs_n       ),
      .rdqs_n  (                 ),
      .odt     ( ddr_odt         )
      );
 `endif // !`elsif NEXYS4
`endif //  `ifdef ADD_PHY_DDR

`ifdef ADD_UART
   wire         rxd;
   wire         txd;
   wire         rts;
   wire         cts;

   assign rxd = 'b1;
   assign cts = 'b1;

`endif

`ifdef ADD_FAN
   wire         fan_pwm;
`endif

`ifdef ADD_FLASH
   wire         flash_ss;
   wire [3:0]   flash_io;

   assign flash_ss = 'bz;
   assign flash_io = 'bzzzz;
`endif

`ifdef ADD_SPI
   wire         spi_cs;
   wire         spi_sclk;
   wire         spi_mosi;
   wire         spi_miso;
   wire         sd_reset;

   assign spi_cs = 'bz;
   assign spi_sclk = 'bz;
   assign spi_mosi = 'bz;
   assign spi_miso = 'bz;
`endif //  `ifdef ADD_SPI

`ifdef ADD_MINION_SD

   // 4-bit full SD interface
   wire         sd_sclk;
   wire         sd_detect;
   wire [3:0]   sd_dat;
   wire         sd_cmd;
   wire         sd_reset;

   sd_card
   sdflash1
     (
      .sdClk ( sd_sclk ),
      .cmd   ( sd_cmd  ),
      .dat   ( sd_dat  )
      );

   // LED and DIP switch
   wire [7:0]   o_led;
   wire [3:0]   i_dip;

   assign i_dip = 'bzzzz;

   // push button array
   wire         GPIO_SW_C;
   wire         GPIO_SW_W;
   wire         GPIO_SW_E;
   wire         GPIO_SW_N;
   wire         GPIO_SW_S;

   assign GPIO_SW_C = 'b1;
   assign GPIO_SW_W = 'b1;
   assign GPIO_SW_E = 'b1;
   assign GPIO_SW_N = 'b1;
   assign GPIO_SW_S = 'b1;

   //keyboard
   wire         PS2_CLK;
   wire         PS2_DATA;

   assign PS2_CLK = 'bz;
   assign PS2_DATA = 'bz;

  // display
   wire        VGA_HS_O;
   wire        VGA_VS_O;
   wire [3:0]  VGA_RED_O;
   wire [3:0]  VGA_BLUE_O;
   wire [3:0]  VGA_GREEN_O;

`endif //  `ifdef FPGA

`ifndef VERILATOR
   // handle all run-time arguments
   string     memfile = "";
   string     vcd_name = "";
   longint    unsigned max_cycle = 0;
   longint    unsigned cycle_cnt = 0;

   initial begin
`ifndef ADD_PHY_DDR
      #5; // wait for the initialization of memory
      if($value$plusargs("load=%s", memfile))
        DUT.ram_behav.memory_load_mem(memfile);
`endif

      $value$plusargs("max-cycles=%d", max_cycle);
   end // initial begin

   // vcd
   initial begin
      if($test$plusargs("vcd"))
        vcd_name = "test.vcd";

      $value$plusargs("vcd_name=%s", vcd_name);

      if(vcd_name != "") begin
         $dumpfile(vcd_name);
         $dumpvars(0, DUT);
         $dumpon;
      end
   end // initial begin

   always @(posedge clk) begin
      cycle_cnt = cycle_cnt + 1;
      if(max_cycle != 0 && max_cycle == cycle_cnt)
        $fatal(0, "maximal cycle of %d is reached...", cycle_cnt);
   end
`endif

`ifdef ADD_HOST
   int rv;
   always @(posedge clk) begin
      rv = DUT.host.check_exit();
      if(rv > 0)
        $fatal(rv);
      else if(rv == 0)
        $finish();
   end
`endif

endmodule // tb
