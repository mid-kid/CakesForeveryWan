.nds

; Variables
sdmmc_unk0			equ 0x08062A28
sdmmc_unk1			equ 0x08078970
sdmmc_unk2			equ 0x08078750

.create "patch1.bin", 0x0801A4C0
.org 0x0801A4C0
.arm
patch000_00:
	stmfd sp!, {r0-r3}
	mov r3, r0
	ldr r1, =orig_sector
	ldr r2, [r3,#4]
	str r2, [r1,#4]
	ldr r0, =0x80D8670
	cmp r2, r0
	ldr r2, [r3,#8]
	str r2, [r1]
	beq @@orig_code
	ldr r1, =0x80D8670
	str r1, [r3,#4]
	cmp r2, #0
	ldr r0, =nand_offset
	ldrne r0, [r0]
	addne r0, r2
	ldreq r0, [r0, #(ncsd_header_offset - nand_offset)]
	str r0, [r3,#8]
@@orig_code:
	ldmfd sp!, {r0-r3}
	movs r4, r0
	movs r5, r1
	movs r7, r2
	movs r6, r3
	movs r0, r1, lsl#23
	beq loc_801a534
	stmfd sp!, {r4}
	ldr r4, =(sdmmc_unk0 + 1)
	blx r4
	ldmfd sp!, {r4}
loc_801a534:
	ldr r0, [r4,#4]
	ldr r1, [r0]
	ldr r1, [r1,#0x18]
	blx r1
	ldr r1, [r4,#4]
	movs r3, r0
	ldr r0, [r1,#0x20]
	movs r2, r5, lsr#9
	mov r12, r0
	ldr r0, [r4,#8]
	str r7, [sp,#4]
	adds r0, r0, r2
	cmp r1, #0
	str r6, [sp,#8]
	str r0, [sp]
	beq loc_801a578
	adds r1, r1, #8
loc_801a578:
	movs r2, r4
	adds r2, r2, #0xc
	mov r0, r12
	ldr r5, =(sdmmc_unk1 + 1) ; called by the original function
	blx r5
	stmfd sp!, {r0-r3}
	ldr r2, =orig_sector
	ldr r1, [r2]
	str r1, [r4,#8]
	ldr r1, [r2,#4]
	str r1, [r4,#4]
	ldmfd sp!, {r0-r3}
	ldmfd sp!, {r1-r7,lr}
	bx lr

patch000_01:
	stmfd sp!, {r0-r3}
	mov r3, r0
	ldr r1, =orig_sector
	ldr r2, [r3,#4]
	str r2, [r1,#4]
	ldr r0, =0x80D8670
	cmp r2, r0
	ldr r2, [r3,#8]
	str r2, [r1]
	beq @@orig_code
	ldr r1, =0x80D8670
	str r1, [r3,#4]
	cmp r2, #0
	ldr r0, =nand_offset
	ldrne r0, [r0]
	addne r0, r2
	ldreq r0, [r0, #(ncsd_header_offset - nand_offset)]
	str r0, [r3,#8]
@@orig_code:
	ldmfd sp!, {r0-r3}
	movs r4, r0
	movs r5, r1
	movs r7, r2
	movs r6, r3
	movs r0, r1, lsl#23
	beq loc_801a624
	stmfd sp!, {r4}
	ldr r4, =(sdmmc_unk0 + 1)
	blx r4
	ldmfd sp!, {r4}
loc_801a624:
	ldr r0, [r4,#4]
	ldr r1, [r0]
	ldr r1, [r1,#0x18]
	blx r1
	ldr r1, [r4,#4]
	movs r3, r0
	ldr r0, [r1,#0x20]
	movs r2, r5, lsr#9
	mov r12, r0
	ldr r0, [r4,#8]
	str r7, [sp,#4]
	adds r0, r0, r2
	cmp r1, #0
	str r6, [sp,#8]
	str r0, [sp]
	beq loc_801a668
	adds r1, r1, #8
loc_801a668:
	movs r2, r4
	adds r2, r2, #0xC
	mov r0, r12
	ldr r5, =(sdmmc_unk2 + 1)
	blx r5
	stmfd sp!, {r0-r3}
	ldr r2, =orig_sector
	ldr r1, [r2]
	str r1, [r4,#8]
	ldr r1, [r2,#4]
	str r1, [r4,#4]
	ldmfd sp!, {r0-r3}
	ldmfd sp!, {r1-r7,lr}
	bx lr

.pool
orig_sector:		.word 0x00000000
orig_ptr:			.word 0x00000000
nand_offset:		.ascii "NAND"       ; for rednand this should be 1
ncsd_header_offset:	.ascii "NCSD"       ; depends on nand manufacturer + emunand type (GW/RED)
;ncsd_header_offset: .word 0x1D7800
;ncsd_header_offset: .word 0x1DD000
slot0x25keyX:
.ascii "slot0x25keyXhere"
.close

.create "patch2.bin", 0x0801B564
.org 0x0801B564
.arm
	.word 0x360003
	.word 0x10100000
	.word 0x1000001
	.word 0x360003
	.word 0x20000035
	.word 0x1010101
	.word 0x200603
	.word 0x8000000
	.word 0x1010101
	.halfword 0x0603
	.byte 0x1C
.close

.create "patch3.bin", 0x080282F8
.org 0x080282F8
.thumb
	ldr r2, =slot0x25keyX
	mov r1, #5
	mov r0, #0x25
	bl 0x80575B4
	bl 0x805FB48
	.halfword 0xBD70		;pop {r4-r6, pc}
.pool
.close

.create "patch4.bin", 0x0807882C
.org 0x0807882C
.thumb
	ldr r4, =patch000_00
	bx r4
.pool
.close

.create "patch5.bin", 0x0807886C
.org 0x0807886C
.thumb
	ldr r4, =patch000_01
	bx r4
.pool
.close
