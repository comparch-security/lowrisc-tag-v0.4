README for Gensys-2 board
========================================================

Requirement:

  **Vivado 2018.3**

How to run the demo:
--------------------------------------------------------

* Generate bit-stream for downloading

        make bitstream

* Run FPGA simulation (extremely slow due to the DDR3 memory controller)

        make simulation

* Open the Vivado GUI

        make vivado

Run SoCDebug:
--------------------------------------------------------

        # terminal 1
        opensocdebugd uart device=/dev/ttyUSB0 speed=3000000
        
        # terminal 2
        osd-cli
        osd> reset -halt
        osd> terminal 2
        osd> mem loadelf boot.bin 3
        osd> start


Other Make targets
--------------------------------------------------------

* Generate the FPGA backend Verilog files

        make verilog

* Generate the Vivado project

        make project

* Find out the boot BRAMs' name and position (for updating src/boot.bmm)

        make search-ramb

* Replace the content of boot BRAM with a new src/boot.mem (must update src/boot.bmm first)

        make bit-update
