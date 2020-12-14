# on board differential clock, 200MHz
set_property PACKAGE_PIN AD12 [get_ports clk_p]
set_property IOSTANDARD DIFF_SSTL15 [get_ports clk_p]
set_property PACKAGE_PIN AD11 [get_ports clk_n]
set_property IOSTANDARD DIFF_SSTL15 [get_ports clk_n]

# Reset active high SW4.1 User button South
set_property VCCAUX_IO DONTCARE [get_ports {rst_top}]
set_property SLEW FAST [get_ports {rst_top}]
set_property IOSTANDARD LVCMOS15 [get_ports {rst_top}]
set_property LOC AB12 [get_ports {rst_top}]

# UART Pins
set_property PACKAGE_PIN M19 [get_ports rxd]
set_property IOSTANDARD LVCMOS25 [get_ports rxd]
set_property PACKAGE_PIN K24 [get_ports txd]
set_property IOSTANDARD LVCMOS25 [get_ports txd]
set_property PACKAGE_PIN K23 [get_ports cts]
set_property IOSTANDARD LVCMOS25 [get_ports cts]
set_property PACKAGE_PIN L27 [get_ports rts]
set_property IOSTANDARD LVCMOS25 [get_ports rts]

# SD/SPI Pins
set_property PACKAGE_PIN AC21 [get_ports spi_cs]
set_property IOSTANDARD LVCMOS25 [get_ports spi_cs]
set_property PACKAGE_PIN AB23 [get_ports spi_sclk]
set_property IOSTANDARD LVCMOS25 [get_ports spi_sclk]
set_property PACKAGE_PIN AB22 [get_ports spi_mosi]
set_property IOSTANDARD LVCMOS25 [get_ports spi_mosi]
set_property PACKAGE_PIN AC20 [get_ports spi_miso]
set_property IOSTANDARD LVCMOS25 [get_ports spi_miso]
#led8
#set_property PACKAGE_PIN F16 [get_ports sd_reset]
#set_property IOSTANDARD LVCMOS25 [get_ports sd_reset]

# Flash/SPI Pins
set_property PACKAGE_PIN U19 [get_ports flash_ss]
set_property IOSTANDARD LVCMOS25 [get_ports flash_ss]
set_property PACKAGE_PIN P24 [get_ports flash_io[0]]
set_property IOSTANDARD LVCMOS25 [get_ports flash_io[0]]
set_property PACKAGE_PIN R25 [get_ports flash_io[1]]
set_property IOSTANDARD LVCMOS25 [get_ports flash_io[1]]
set_property PACKAGE_PIN R20 [get_ports flash_io[2]]
set_property IOSTANDARD LVCMOS25 [get_ports flash_io[2]]
set_property PACKAGE_PIN R21 [get_ports flash_io[3]]
set_property IOSTANDARD LVCMOS25 [get_ports flash_io[3]]

#led
set_property PACKAGE_PIN AB8 [get_ports led[0]]
set_property IOSTANDARD LVCMOS15 [get_ports led[0]]
set_property PACKAGE_PIN AA8 [get_ports led[1]]
set_property IOSTANDARD LVCMOS15 [get_ports led[1]]
set_property PACKAGE_PIN AC9 [get_ports led[2]]
set_property IOSTANDARD LVCMOS15 [get_ports led[2]]
set_property PACKAGE_PIN AB9 [get_ports led[3]]
set_property IOSTANDARD LVCMOS15 [get_ports led[3]]
set_property PACKAGE_PIN AE26 [get_ports led[4]]
set_property IOSTANDARD LVCMOS25 [get_ports led[4]]
set_property PACKAGE_PIN G19 [get_ports led[5]]
set_property IOSTANDARD LVCMOS25 [get_ports led[5]]
set_property PACKAGE_PIN E18 [get_ports led[6]]
set_property IOSTANDARD LVCMOS25 [get_ports led[6]]
set_property PACKAGE_PIN F16 [get_ports led[7]]
set_property IOSTANDARD LVCMOS25 [get_ports led[7]]

# Set DCI_CASCADE for DDR3 interface
set_property slave_banks {32 34} [get_iobanks 33]


#set_property CLOCK_DEDICATED_ROUTE BACKBONE [get_nets clk_gen/inst/clk_in1_wiz_0]
