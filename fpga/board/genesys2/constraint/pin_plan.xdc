# on board differential clock, 200MHz
set_property PACKAGE_PIN AD12 [get_ports clk_p]
set_property IOSTANDARD LVDS [get_ports clk_p]
set_property PACKAGE_PIN AD11 [get_ports clk_n]
set_property IOSTANDARD LVDS [get_ports clk_n]

# Reset active high SW4.1 User button South
set_property VCCAUX_IO DONTCARE [get_ports {rst_top}]
set_property SLEW FAST [get_ports {rst_top}]
set_property IOSTANDARD LVCMOS33 [get_ports {rst_top}]
set_property LOC R19 [get_ports {rst_top}]

# UART Pins
set_property PACKAGE_PIN Y20 [get_ports rxd]
set_property IOSTANDARD LVCMOS33 [get_ports rxd]
set_property PACKAGE_PIN Y23 [get_ports txd]
set_property IOSTANDARD LVCMOS33 [get_ports txd]
#set_property PACKAGE_PIN K23 [get_ports cts]
#set_property IOSTANDARD LVCMOS25 [get_ports cts]
#set_property PACKAGE_PIN L27 [get_ports rts]
#set_property IOSTANDARD LVCMOS25 [get_ports rts]

# SD/SPI Pins
set_property PACKAGE_PIN T30 [get_ports spi_cs]
set_property IOSTANDARD LVCMOS33 [get_ports spi_cs]
set_property PACKAGE_PIN R28 [get_ports spi_sclk]
set_property IOSTANDARD LVCMOS33 [get_ports spi_sclk]
set_property PACKAGE_PIN R29 [get_ports spi_mosi]
set_property IOSTANDARD LVCMOS33 [get_ports spi_mosi]
set_property PACKAGE_PIN R26 [get_ports spi_miso]
set_property IOSTANDARD LVCMOS33 [get_ports spi_miso]
set_property PACKAGE_PIN AE24 [get_ports sd_reset]
set_property IOSTANDARD LVCMOS33 [get_ports sd_reset]

# Flash/SPI Pins
set_property PACKAGE_PIN U19 [get_ports flash_ss]
set_property IOSTANDARD LVCMOS33 [get_ports flash_ss]
set_property PACKAGE_PIN P24 [get_ports flash_io[0]]
set_property IOSTANDARD LVCMOS33 [get_ports flash_io[0]]
set_property PACKAGE_PIN R25 [get_ports flash_io[1]]
set_property IOSTANDARD LVCMOS33 [get_ports flash_io[1]]
set_property PACKAGE_PIN R20 [get_ports flash_io[2]]
set_property IOSTANDARD LVCMOS33 [get_ports flash_io[2]]
set_property PACKAGE_PIN R21 [get_ports flash_io[3]]
set_property IOSTANDARD LVCMOS33 [get_ports flash_io[3]]

#led
set_property PACKAGE_PIN T28 [get_ports led[0]]
set_property IOSTANDARD LVCMOS33 [get_ports led[0]]
set_property PACKAGE_PIN V19 [get_ports led[1]]
set_property IOSTANDARD LVCMOS33 [get_ports led[1]]
set_property PACKAGE_PIN U30 [get_ports led[2]]
set_property IOSTANDARD LVCMOS33 [get_ports led[2]]
set_property PACKAGE_PIN U29 [get_ports led[3]]
set_property IOSTANDARD LVCMOS33 [get_ports led[3]]
set_property PACKAGE_PIN V20 [get_ports led[4]]
set_property IOSTANDARD LVCMOS33 [get_ports led[4]]
set_property PACKAGE_PIN V26 [get_ports led[5]]
set_property IOSTANDARD LVCMOS33 [get_ports led[5]]
set_property PACKAGE_PIN W24 [get_ports led[6]]
set_property IOSTANDARD LVCMOS33 [get_ports led[6]]
set_property PACKAGE_PIN W23 [get_ports led[7]]
set_property IOSTANDARD LVCMOS33 [get_ports led[7]]

