.nds

.create "signatures.cake", 0

; Header
.byte 2  ; Amount of patches
.byte 0x49  ; Firmware v9.6
.byte 0  ; Reserved
.byte patches  ; Offset to start of patches
.ascii "Disable signature checks", 0

patches:

; Patch structure
; - Address
; - Offset
; - Size
; - Options

; Patch 1
.word 0x080632B8
.word patch1
.word patch1_end - patch1
.byte 0

; Patch 2
.word 0x0805D628
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
