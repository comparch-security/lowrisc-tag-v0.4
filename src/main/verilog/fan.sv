// See LICENSE for license details.

module fan_ctl
  #(
    ID_WIDTH = 1,               // id width
    DATA_WIDTH = 32,            // data width
    USER_WIDTH = 1              // width of user field
    )
   (
    input logic           clk, rstn,
    input logic [11:0]    device_temp,
    output logic          fan_pwm,
    nasti_channel.slave   nasti
    );

   assign nasti.aw_ready = 0;
   assign nasti.b_valid  = 0;

   logic                  ar_fire;
   logic [ID_WIDTH-1:0]   r_id;
   logic [USER_WIDTH-1:0] r_user;
   logic [2:0]            r_addr;

   always_ff @(posedge clk or negedge rstn)
   if(!rstn)
     ar_fire <= 0;
   else if(nasti.ar_valid && nasti.ar_ready) begin
      ar_fire <= 1;
      r_id <= nasti.ar_id;
      r_user <= nasti.ar_user;
      r_addr <= nasti.ar_addr;
   end else
     ar_fire <= 0;

   assign nasti.ar_ready = !ar_fire;
   assign nasti.r_valid  = ar_fire;

   assign nasti.r_id = r_id;
   assign nasti.r_resp = 0;
   assign nasti.r_user = r_user;
   assign nasti.r_last = 1;
   logic [DATA_WIDTH-1:0] r_data;
   logic [DATA_WIDTH-1:0] temperature;
   assign nasti.r_data = r_data;

   always @(r_addr)
     case(r_addr)
        3'b000: r_data = device_temp;
        3'b100: r_data = temperature;
        default: r_data = device_temp;
     endcase

   // Xilinx UG480: 7-series XADC
   // temp = (device_temp * 503.975) / 4096 - 273.15
   // Here it is implemented as:
   //   temp = (device_temp - 2220) * 126 / 1024
   assign temperature = ((device_temp - 32'd2220) * 126) >> 10;

   localparam cbit = 8;
   logic [cbit-1:0] cnt;
   

   always @(posedge clk or negedge rstn)
     if(!rstn)
       cnt = 0;
     else
       cnt = cnt + 1;

   always @(cnt, temperature)
     if     (temperature > 60) fan_pwm = 1;
     else if(temperature > 55) fan_pwm = cnt > (1 << cbit) * 80 / 100;
     else if(temperature > 50) fan_pwm = cnt > (1 << cbit) * 85 / 100;
     else if(temperature > 48) fan_pwm = cnt > (1 << cbit) * 90 / 100;
     else if(temperature > 45) fan_pwm = cnt > (1 << cbit) * 95 / 100;
     else                      fan_pwm = 0;

endmodule
