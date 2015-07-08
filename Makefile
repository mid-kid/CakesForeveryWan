rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

CC := arm-none-eabi-gcc
AS := arm-none-eabi-as
LD := arm-none-eabi-ld
OC := arm-none-eabi-objcopy
OPENSSL := openssl

PYTHON3 := python
PYTHON_VER_MAJOR := $(word 2, $(subst ., , $(shell python --version 2>&1)))
ifneq ($(PYTHON_VER_MAJOR), 3)
	PYTHON3 := python3
endif

dir_source := source
dir_build := build
dir_out := out
dir_patches := patches
dir_cakehax := CakeHax

dir_dhs := tools/dhs
dir_dhs_client := tools/dhs_client

ASFLAGS := -mlittle-endian -mcpu=arm946e-s -march=armv5te
CFLAGS := -MMD -MP -marm $(ASFLAGS) -fno-builtin -fshort-wchar -Wall -Wextra -O2 -std=c11 -Wno-main

revision := $(shell git rev-list HEAD --count)

objects_cfw = $(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
			  $(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
			  $(call rwildcard, $(dir_source), *.s *.c)))

baked_files := $(patsubst $(dir_patches)/%/, $(dir_build)/patches/%.baked, $(wildcard $(dir_patches)/*/))

provide_files := $(dir_out)/firmware_bin.here \
				 $(dir_out)/slot0x25keyX_bin.here \
				 $(dir_out)/cakes/firmkey_bin.here

.PHONY: all
all: launcher patches dhs

.PHONY: release
release: Cakes_$(revision).zip

.PHONY: launcher
launcher: $(dir_out)/Cakes.dat

.PHONY: patches
patches: $(baked_files)

.PHONY: dhs
dhs:
	@make -C $(dir_dhs) all
	@make -C $(dir_dhs_client) all

.PHONY: clean
clean:
	@make dir_out=$(abspath $(dir_out)) -C $(dir_cakehax) clean
	@make -C $(dir_dhs) clean
	@make -C $(dir_dhs_client) clean
	rm -rf $(dir_out) $(dir_build) Cakes_$(revision).zip

$(dir_out)/%.here:
	@mkdir -p "$(@D)"
	touch $@

Cakes_$(revision).zip: launcher patches $(provide_files)
	sh -c "cd $(dir_out); zip -r ../$@ *"

$(dir_out)/Cakes.dat: $(dir_build)/main.bin
	@mkdir -p "$(@D)"
	@make dir_out=$(abspath $(@D)) -C $(dir_cakehax) launcher
	dd if=$(dir_build)/main.bin of=$@ bs=512 seek=256

$(dir_build)/patches/%.baked: $(dir_patches)/%/info.json $(dir_patches)/%/patches.s
	@mkdir -p $(dir_out)/cakes/patches
	@mkdir -p $(dir_build)/patches/$*
	$(PYTHON3) $(dir_patches)/bundle.py $^ $(dir_build)/patches/$* $(dir_out)/cakes/patches
	@touch $@

$(dir_build)/main.bin: $(dir_build)/main.elf
	$(OC) -S -O binary $< $@

$(dir_build)/main.elf: $(objects_cfw)
	# FatFs requires libgcc for __aeabi_uidiv
	$(CC) -nostartfiles $(LDFLAGS) -T linker.ld $(OUTPUT_OPTION) $^

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
