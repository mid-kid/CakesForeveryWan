.section .text.a11.entry
.arm

.global a11compat
a11compat:
buffer_addr:
	.word 0xFFF3F000
translate_va:
	.word 0xFFF7A8C4
mmu_table_offset:
	.word 0x54
codeset_offset:
	.word 0xA8
exit_process:
	.word 0xFFF72FCC

.global dataabort_hook
dataabort_hook:
	srsdb sp!, #0x13			@ Store return state on SVC stack
	cps #0x13					@ Switch to SVC mode
	str lr, [sp, #-4]!			@ Store the lr for the current mode (pc for user mode)
	ldr lr, [sp, #4]
	sub lr, lr, #8
	str lr, [sp, #-4]!			@ Why?
	mov lr, #2					@ Don't need this actually
	str lr, [sp, #-4]!
	b handler_wrapper

handler_wrapper:
	stmfd sp, {sp,lr}^			@ Store sp and lr for the user mode
	sub sp, sp, #8				@ Don't need this actually
	stmfd sp!, {r0-r12,lr}
	cpsie af					@ Enable abort, FIQ
	mov r0, sp
	bl abortHookHandler
	cpsid aif					@ Disable abort, IRQ, FIQ
	ldmfd sp!, {r0-r12,lr}
	ldmfd sp, {sp,lr}^
	add sp, sp, #8
	add sp, sp, #0xC
	rfefd sp!					@ Return from the SVC mode stack
.pool

.global get_compat
get_compat:
	adr r0, a11compat
	bx lr
.pool

.global ld11_hook
ld11_hook:
	mov r5, r0					@ The code we patched
	stmfd sp!, {r0-r12,lr}
	cpsid i
	ldr r0, [sp, #0x04]
	ldr r1, [sp, #0x08]
	bl ldHookHandler
	cpsie i
	ldmfd sp!, {r0-r12,pc}
.pool

.global ssr_hook
ssr_hook:
	stmfd sp!, {r0-r3,lr}
	bl ssrHookHandler
	ldmfd sp!, {r0-r3,lr}
	sub sp, sp, #0x14			@ The code we patched
	bx lr
.pool
