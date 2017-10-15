.arm.little

.create "b9s_screens.bin", 0x24000000
.arm
    cmp r0, 2
    blo run

    ldr r0, [r1, #4]

    ldr r1, [r0]
    ldr r2, [r0, #4]
    ldr r3, [r0, #8]

    ldr r0, =0x23FFFE00
    str r1, [r0]
    str r2, [r0, #4]
    str r3, [r0, #8]

run:
    ldr r0, =0x23F00000
    bx r0
.pool
.close
