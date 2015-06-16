OUTPUT_ARCH(arm)
ENTRY(_entry)
SECTIONS
{
	. = 0x01FF8000;
	.text : {
		*(.text.entry)
		
		_a11_hook_start = .;
		*(.text.a11.entry)
		*(.text.a11)
		_a11_hook_end = .;
		
		*(.text)
	}
	.data : {
		*(.data)
	}
	.bss : {
		*(.bss COMMON)
	}
	.rodata : {
		*(.rodata)
	}
	. = ALIGN(4);
}