.nds

.create "patch1.bin"
.thumb
mov r0, #0
.close

.create "patch2.bin"
.thumb
mov r0, #0
bx lr
.close
