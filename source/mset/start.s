.section .text.start
.global _start
_start:
    @ Make sure this doesn't run more than once
    #mov r0, #0
    #ldr r1, =run_once
    #ldr r2, [r1]
    #cmp r0, r2
    #beq .die
    #str r0, [r1]

    bl main

.die:
    b .die
