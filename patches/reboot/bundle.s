.nds

.create "reboot.cake", 0

; Header
.byte 2  ; Amount of patches
.byte patches
.ascii "Fix reboot in emuNAND", 0

patches:

.word 0x080859C8
.word patch1
.word patch1_end - patch1
.byte 4  ; save

.word 0x08094454
.word patch2
.word patch2_end - patch2
.byte 0

patch1:
.incbin "patch1.bin"
patch1_end:

patch2:
.incbin "patch2.bin"
patch2_end:

.close
