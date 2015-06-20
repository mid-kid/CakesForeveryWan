.nds

.create "dhs.cake", 0

; Header
.byte 2  ; Amount of patches
.byte patches  ; Offset to start of patches
.ascii "DHS", 0

patches:

; Patch structure
; - Address
; - Offset
; - Size
; - Options

; Patch 1
.word 0x08086140
.word patch1
.word patch1_end - patch1
.byte 0

; Patch 2
.word 0x08086174
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
