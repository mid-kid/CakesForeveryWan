.nds

.create "emunand.cake", 0

; Header
.byte 5  ; Amount of patches
.byte 0x49  ; Firmware v9.6
.byte 0  ; Reserved
.byte patches
.ascii "Enable emuNAND", 0

patches:

.word 0x0801A4C0
.word patch1
.word patch1_end - patch1
.byte 3  ; keyx | emunand

.word 0x0801B564
.word patch2
.word patch2_end - patch2
.byte 0

.word 0x080282F8
.word patch3
.word patch3_end - patch3
.byte 0

.word 0x0807882C
.word patch4
.word patch4_end - patch4
.byte 0

.word 0x0807886C
.word patch5
.word patch5_end - patch5
.byte 0

patch1:
.incbin "patch1.bin"
patch1_end:

patch2:
.incbin "patch2.bin"
patch2_end:

patch3:
.incbin "patch3.bin"
patch3_end:

patch4:
.incbin "patch4.bin"
patch4_end:

patch5:
.incbin "patch5.bin"
patch5_end:

.close
