.section .text.entry, "x"
.arm
.align 4

.global _entry
_entry:
	b main

.global a9compat
a9compat:
	.global hook_load
	hook_load:
		.word 0
	.global hook_return
	hook_return:
		.word 0
	.space (20 - 2) * 4

.global dump_only
dump_only:
	.word 0
.pool
