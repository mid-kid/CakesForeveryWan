OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

/*0x080C3EE0*/
ENTRY(_start)

SECTIONS
{
  /*. = 0x080C3EE0;*/
  . = 0x00240000;
  start_addr = .;
  .text.start : { *(.text.start) }
  .text       : { *(.text) *(.text*) }
  .rodata     : { *(.rodata) *(.rodata*) }
  .data       : { *(.data) *(.data*) }
  .bss        : { *(.bss) *(.bss*) }
  . = ALIGN(32);
  /*.stack : {
    stack_start = .;
    . += 0x400;
    . = ALIGN(32);
    stack_end = .;
  }*/
  total_size = . - start_addr;
}
