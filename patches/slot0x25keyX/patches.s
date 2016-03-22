.arm.little

.create "patch1.bin", 0
.arm
set_slot0x25:
    adr r2, slot0x25keyX
    mov r1, #5
    mov r0, #0x25

    ldr r6, [aes_setkey]
    orr r6, 1
    blx r6
    ldr r6, [aes_unk]
    orr r6, 1
    blx r6

    pop {r4-r6, lr}
    bx lr
.align 4
slot0x25keyX: .ascii "slot0x25keyXhere"
.align 4
aes_setkey: .ascii "setkey"
.align 4
aes_unk: .ascii "unk"
.pool
.close

.create "patch2.bin", 0
.thumb
    ldr r2, [_set_slot0x25]
    blx r2
.align 4
_set_slot0x25: .ascii "mem"
.close
