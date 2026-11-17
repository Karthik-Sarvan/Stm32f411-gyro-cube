MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 128K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
  .text : { *(.text*) *(.rodata*) KEEP(*(.init)) KEEP(*(.fini)) } > FLASH
  .ARM.exidx : { *(.ARM.exidx*) } > FLASH
  .data : AT(LOADADDR(.ARM.exidx) + SIZEOF(.ARM.exidx))
  {
    _sdata = .; *(.data*) _edata = .;
  } > RAM
  _sidata = LOADADDR(.data);
  .bss : { _sbss = .; *(.bss*) *(COMMON) _ebss = .; } > RAM
  ._user_heap_stack :
  {
    . = ALIGN(8); . = . + 0x800; . = . + 0x800; . = ALIGN(8);
  } > RAM
}
