@ Copyright (c) 2015, Shiny Quagsire, WulfyStylez, Dazzozo, and contributers
@ All rights reserved.

@ Redistribution and use in source and binary forms, with or without
@ modification, are permitted provided that the following conditions are met:
@ 1. Redistributions of source code must retain the above copyright
@    notice, this list of conditions and the following disclaimer.
@ 2. Redistributions in binary form must reproduce the above copyright
@    notice, this list of conditions and the following disclaimer in the
@    documentation and/or other materials provided with the distribution.
@ 3. All advertising materials mentioning features or use of this software
@    must display the following acknowledgement:
@    This product includes software developed by Shiny Quagsire (and further contributors).
@ 4. Neither the name of the <organization> nor the
@    names of its contributors may be used to endorse or promote products
@    derived from this software without specific prior written permission.

@ THIS SOFTWARE IS PROVIDED BY THE　COPYRIGHT HOLDERS ''AS IS'' AND ANY
@ EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
@ WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
@ DISCLAIMED. IN NO EVENT SHALL THE　COPYRIGHT HOLDERS BE LIABLE FOR ANY
@ DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
@ (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
@ LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
@ ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
@ (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
@ SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


@ This file is taken from the bootstrap project.
@ The reasoning behind it is because it works as-is, and I have no intention nor need
@   to edit it.
@ Possible TODO someday: Rewrite this to make it readable.
@   I'm sorry, but this looks like partially deobfuscated, reverse engineered code.


.arm
.align 4
.code 32
.text

.globl jump_table
jump_table:
		B	newloc_func_patch_hook
		B	newloc_reboot_func

newloc_func_patch_hook:

		STMFD	SP!, {R0-R12,LR}
		MOV	R0, #0
		BL	sub_9D0470_9D253C
		BL	sub_9D0470_9D2554
		MOV	R0, #0x10000
		BL	sub_9D0470_9D253C
		BL	sub_9D0470_9D2568
		BL	sub_9D0470_9D2568
		BL	sub_9D0470_9D2568

		LDR	R1, jt_pdn_regs
		MOV	R0, #2
		STRB	R0, [R1,#0x230]
		MOV	R0, #0x10
		BL	sub_9D0470_9D252C
		MOV	R0, #0
		STRB	R0, [R1,#0x230]
		MOV	R0, #0x10
		BL	sub_9D0470_9D252C
		LDMFD	SP!, {R0-R12,LR}

		STMFD	SP!, {R0-R12,LR}
		LDR  R1, FB_1
		MOV  R0, #0xFF
		STRB R0, [R1]
		LDR  R1, FB_2
		STRB R0, [R1]
		bl invalidate_all_dcache
		LDMFD	SP!, {R0-R12,LR}

		LDR	R0, dword_9D0470_9D247C
		STR	R0, [R1]
		LDR	PC, jt_func_patch_return

.globl jt_pdn_regs
	jt_pdn_regs:		.long 0xFFFDA008
.globl jt_pxi_regs
	jt_pxi_regs:		.long 0xFFFCC48C
.globl jt_func_patch_return
	jt_func_patch_return:	.long 0xFFF5045C


newloc_reboot_func:	
		ADR	R0, reboot_wait
		ADR	R1, invalidate_all_cache
		LDR	R2, off_9D0470_9D2480
		MOV	R4, R2
		BL	sub_9D0470_9D2430
		BX	R4

sub_9D0470_9D2430:
		SUB	R3, R1,	R0
		MOV	R1, R3,ASR#2
		CMP	R1, #0
		BLE	locret_9D0470_9D2478
		MOVS	R1, R3,LSL#29
		SUB	R0, R0,	#4
		SUB	R1, R2,	#4
		BPL	loc_9D0470_9D2458
		LDR	R2, [R0,#4]!
		STR	R2, [R1,#4]!

loc_9D0470_9D2458:
		MOVS	R2, R3,ASR#3
		BEQ	locret_9D0470_9D2478

loc_9D0470_9D2460:
		LDR	R3, [R0,#4]
		SUBS	R2, R2,	#1
		STR	R3, [R1,#4]
		LDR	R3, [R0,#8]!
		STR	R3, [R1,#8]!
		BNE	loc_9D0470_9D2460

locret_9D0470_9D2478:
		BX	LR

dword_9D0470_9D247C:	.long 0x44836
off_9D0470_9D2480:		.long	0x1FFFFC00

.globl reboot_wait
reboot_wait:
		LDR	R1, dword_0_1FFF49C4
		LDR	R2, dword_0_1FFF49C8
		STR	R2, [R1]

		LDR	R10, firm_exec_ptr
		LDR	R9, arm9_payload
		LDR	R8, reboot_ready
wait_arm9_loop:
		LDRB	R0, [R8]
		ANDS	R0, R0,	#1
		BNE	wait_arm9_loop
		STR	R9, [R10] @ Write our address

		MVN	R0, #0xE0000007 @ Load wait address
wait_arm11_loop:
		LDR	R1, [R0]
		CMP	R1, #0
		BEQ	wait_arm11_loop
		BX	R1

off_0_1FFF49BC:	.long 0x1FFFFC00
dword_0_1FFF49C4:	.long 0x10163008
dword_0_1FFF49C8:	.long 0x44846
firm_exec_ptr:		.long 0x2400000C
arm9_payload:		.long 0x23F00000
reboot_ready:		.long 0x10140000
dword_9D0470_9D24D8:	.long 0x10163008
dword_9D0470_9D24DC:	.long 0x44846
FB_1: .long 0x1408ca37 @Supposed bottom screen buffers
FB_2: .long  0x140c4e37

.align 4

invalidate_all_cache:
		MOV	R0, #0
		MCR	p15, 0,	R0,c8,c5, 0
		MCR	p15, 0,	R0,c8,c6, 0
		MCR	p15, 0,	R0,c8,c7, 0
		MCR	p15, 0,	R0,c7,c10, 4
		BX	LR


invalidate_all_dcache:
		MOV	R0, #0
		MCR	p15, 0,	R0,c7,c14, 0
		MCR	p15, 0,	R0,c7,c10, 4
		BX	LR


invalidate_all_icache:
		MOV	R0, #0
		MCR	p15, 0,	R0,c7,c5, 0
		MCR	p15, 0,	R0,c7,c5, 4
		MCR	p15, 0,	R0,c7,c5, 6
		MCR	p15, 0,	R0,c7,c10, 4
		BX	LR

sub_9D0470_9D252C:
		SUBS	R0, R0,	#2
		NOP
		BGT	sub_9D0470_9D252C
		BX	LR


sub_9D0470_9D253C:
		LDR	R1, jt_pxi_regs
loc_9D0470_9D2540:
		LDRH	R2, [R1,#4]
		TST	R2, #2
		BNE	loc_9D0470_9D2540
		STR	R0, [R1,#8]
		BX	LR

sub_9D0470_9D2554:
		LDR	R0, jt_pxi_regs
		LDRB	R1, [R0,#3]
		ORR	R1, R1,	#0x40
		STRB	R1, [R0,#3]
		BX	LR

sub_9D0470_9D2568:
		LDR	R0, jt_pxi_regs
loc_9D0470_9D256C:
		LDRH	R1, [R0,#4]
		TST	R1, #0x100
		BNE	loc_9D0470_9D256C
		LDR	R0, [R0,#0xC]
		BX	LR

.globl jump_table_end
jump_table_end: .long 0

