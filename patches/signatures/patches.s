.nds

.create "patch1.bin", 0x080632B8
.org 0x080632B8
.thumb
mov r0, #0
.close

.create "patch2.bin", 0x0805D628
.org 0x0805D628
.thumb
mov r0, #0
bx lr
.close
