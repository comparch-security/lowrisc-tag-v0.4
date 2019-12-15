OUTPUT_ARCH( "riscv" )

SECTIONS
{
  . = 0x40000000;
  .text.init : { *(.text.init) }
  .text : { *(.text) }
  .data : { *(.data) }
  .bss : { *(.bss) }
  _end = .;
}

