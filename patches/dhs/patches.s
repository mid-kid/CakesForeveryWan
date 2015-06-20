.nds

.create "patch1.bin", 0x08086140
.org 0x08086140
.arm
ldr pc, =0x01FF8000
.close

.create "patch2.bin", 0x08086174
.org 0x08086174
.pool
.close
