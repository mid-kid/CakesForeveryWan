.nds

.create "patch1.bin", 0
.thumb
mov r0, #0
.close

.create "patch2.bin", 0
.thumb
mov r0, #0
bx lr
.close
