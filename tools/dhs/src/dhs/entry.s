.section .text.entry, "x"

.align 4
.global _entry
_entry:
	b main
.pool

.global __service_ptr
__service_ptr:
	.word 0
.global __heap_size
__heap_size:
	.word 0x88000
.global __linear_heap_size
__linear_heap_size:
	.word 0xC0000
