OUTPUT_ARCH(arm)
ENTRY(_entry)
SECTIONS
{
	. = 0x00100000;
	.text : {
		*(.text.entry)
		*(.text)
	}
	
	. = 0x00107000;
	.data ALIGN(4) : {
		*(.data)
	}
	.bss ALIGN(4) : {
		*(.bss COMMON)
	}
	.rodata ALIGN(4) : {
		*(.rodata)
	}
	. = ALIGN(4);
	__end__ = .;
}