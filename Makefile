rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

CC := arm-none-eabi-gcc
AS := arm-none-eabi-as
LD := arm-none-eabi-ld
OC := arm-none-eabi-objcopy
OPENSSL := openssl

PYTHON := python

dir_source := source
dir_build := build
dir_out := out
dir_patches := patches
dir_cakehax := CakeHax
dir_cakebrah := CakeBrah

ASFLAGS := -mlittle-endian -mcpu=arm946e-s -march=armv5te
CFLAGS := -MMD -MP -marm $(ASFLAGS) -fno-builtin -fshort-wchar -Wall -Wextra -O2 -std=c11 -Wno-main
CAKEFLAGS := dir_out=$(abspath $(dir_out))
BRAHFLAGS := dir_out=$(abspath $(dir_out)/3ds/Cakes) \
			 APP_DESCRIPTION="CFW for 3DS" \
			 APP_AUTHOR="mid-kid" \
			 ICON=$(abspath icon.png)

revision := $(shell git rev-list HEAD --count)

objects_cfw = $(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
			  $(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
			  $(call rwildcard, $(dir_source), *.s *.c)))

baked_files := $(patsubst $(dir_patches)/%/, $(dir_build)/patches/%.baked, $(wildcard $(dir_patches)/*/))

provide_files := $(dir_out)/slot0x25keyX_bin.here \
				 $(dir_out)/cakes/firmware_bin.here \
				 $(dir_out)/cakes/cetk.here

.PHONY: all
all: launcher patches ninjhax

.PHONY: release
release: Cakes_$(revision).zip

.PHONY: launcher
launcher: $(dir_out)/Cakes.dat

.PHONY: patches
patches: $(baked_files)

.PHONY: ninjhax
ninjhax: $(dir_out)/3ds/Cakes/Cakes.3dsx

.PHONY: clean
clean:
	@$(MAKE) $(CAKEFLAGS) -C $(dir_cakehax) clean
	@$(MAKE) $(BRAHFLAGS) -C $(dir_cakebrah) clean
	rm -rf $(dir_out) $(dir_build) Cakes_$(revision).zip

Cakes_$(revision).zip: launcher patches ninjhax $(provide_files)
	sh -c "cd $(dir_out); zip -r ../$@ *"

$(dir_out)/%.here:
	@mkdir -p "$(@D)"
	touch $@

.PHONY: $(dir_out)/Cakes.dat
$(dir_out)/Cakes.dat: $(dir_build)/main.bin
	@mkdir -p "$(@D)"
	@$(MAKE) $(CAKEFLAGS) -C $(dir_cakehax) launcher
	dd if=$(dir_build)/main.bin of=$@ bs=512 seek=144

$(dir_build)/patches/%.baked: $(dir_patches)/%/info.json $(dir_patches)/%/patches.s
	@mkdir -p $(dir_out)/cakes/patches
	@mkdir -p $(dir_build)/patches/$*
	$(PYTHON) $(dir_patches)/bundle.py $^ $(dir_build)/patches/$* $(dir_out)/cakes/patches
	@touch $@

.PHONY: $(dir_out)/3ds/Cakes/Cakes.3dsx $(dir_out)/3ds/Cakes/Cakes.smdh
$(dir_out)/3ds/Cakes/Cakes.3dsx $(dir_out)/3ds/Cakes/Cakes.smdh:
	@mkdir -p "$(@D)"
	@$(MAKE) $(BRAHFLAGS) -C $(dir_cakebrah) all

$(dir_build)/main.bin: $(dir_build)/main.elf
	$(OC) -S -O binary $< $@

$(dir_build)/main.elf: $(objects_cfw)
	# FatFs requires libgcc for some optimizations
	$(LD) -T linker.ld  $(OUTPUT_OPTION) $^ $(shell $(CC) -print-libgcc-file-name)

$(dir_build)/%.o: $(dir_source)/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(dir_build)/%.o: $(dir_source)/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.s) $(OUTPUT_OPTION) $<

$(dir_build)/fatfs/%.o: $(dir_source)/fatfs/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) -mthumb -mthumb-interwork -Wno-unused-function $(OUTPUT_OPTION) $<

$(dir_build)/fatfs/%.o: $(dir_source)/fatfs/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.s) -mthumb -mthumb-interwork $(OUTPUT_OPTION) $<

include $(call rwildcard, $(dir_build), *.d)
