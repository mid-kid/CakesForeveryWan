.arm.little

#!variables

.create "patch1.bin"
slot0x25keyX: .ascii "slot0x25keyXhere"
.close

.create "patch2.bin"
.thumb
	ldr r2, =slot0x25keyX
	mov r1, #5
	mov r0, #0x25
	bl aes_setkey
	bl aes_unk
	.halfword 0xBD70		;pop {r4-r6, pc}
.pool
.close
