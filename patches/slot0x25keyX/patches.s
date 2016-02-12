.nds

.create "patch1.bin", 0
.thumb
set_slot0x25:
    ldr r2, =slot0x25keyX
    mov r1, #5
    mov r0, #0x25

    ldr r4, [aes_setkey]
    blx r4
    ldr r4, [aes_unk]
    blx r4

    pop {r4-r6, pc}
slot0x25keyX: .ascii "slot0x25keyXhere"
.align 4
aes_setkey: .ascii "setkey"
.align 4
aes_unk: .ascii "unk"
.pool
.close

.create "patch2.bin", 0
.thumb
    ldr r4, [_set_slot0x25]
    bx r4
.align 4
_set_slot0x25: .ascii "mem"
.close
