.nds

.create "patch1.bin"
.arm
ldr pc, =0x01FF8000
.close

.create "patch2.bin"
.pool
.close
