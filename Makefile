rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

# Either have git installed or replace this thing for the revisision to show up.
revision := $(shell git rev-list HEAD --count)
ifeq ($(revision),)
	revision := "from another dimension"
endif

CC := arm-none-eabi-gcc
AS := arm-none-eabi-as
LD := arm-none-eabi-ld
OC := arm-none-eabi-objcopy

PYTHON := python3
CONVERT := convert
ARMIPS := armips

dir_source := source
dir_build := build
dir_out := out
dir_patches := patches
dir_hacks := hacks
dir_cakehax := CakeHax
dir_cakebrah := CakeBrah

ASFLAGS := -mlittle-endian -mcpu=arm946e-s -march=armv5te
LDFLAGS := -flto -fno-builtin -fshort-wchar
CFLAGS := $(ASFLAGS) $(LDFLAGS) -marm -MMD -MP -Wall -Wextra -O2 -std=c11 -Wno-main -DCAKES_VERSION=\"$(revision)\"
CAKEFLAGS := dir_out=$(abspath $(dir_out))
BRAHFLAGS := dir_out=$(abspath $(dir_out)/3ds/Cakes) \
			 APP_DESCRIPTION="CFW for 3DS" \
			 APP_AUTHOR="mid-kid" \
			 ICON=$(abspath icon.png)

objects_cfw = $(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
			  $(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
			  $(patsubst $(dir_source)/%.mono, $(dir_build)/%.o, \
			  $(call rwildcard, $(dir_source), *.s *.c *.mono))))

cakes := $(patsubst $(dir_patches)/%/, $(dir_out)/cakes/patches/%.cake, $(wildcard $(dir_patches)/*/))

provide_files := $(dir_out)/cakes/firmware_bin.here \
				 $(dir_out)/cakes/cetk.here

.PHONY: all
all: launcher launcher_bin launcher_firm patches ninjhax

.PHONY: release
release: Cakes_$(revision).zip

.PHONY: launcher
launcher: $(dir_out)/Cakes.dat

.PHONY: launcher_bin
launcher_bin: $(dir_out)/Cakes.bin

.PHONY: launcher_firm
launcher_firm: $(dir_out)/Cakes.firm

.PHONY: patches
patches: $(cakes)

.PHONY: ninjhax
ninjhax: $(dir_out)/3ds/Cakes/Cakes.3dsx

.PHONY: clean
clean:
	@$(MAKE) $(CAKEFLAGS) -C $(dir_cakehax) clean
	@$(MAKE) $(BRAHFLAGS) -C $(dir_cakebrah) clean
	rm -rf $(dir_out) $(dir_build) Cakes_$(revision).zip

Cakes_$(revision).zip: launcher launcher_bin launcher_firm patches ninjhax $(provide_files)
	sh -c "cd $(dir_out); zip -r ../$@ *"

$(dir_out)/%.here:
	@mkdir -p "$(@D)"
	touch $@

.PHONY: $(dir_out)/Cakes.dat
$(dir_out)/Cakes.dat: $(dir_out)/Cakes.bin
	@mkdir -p "$(@D)"
	@$(MAKE) $(CAKEFLAGS) -C $(dir_cakehax) launcher
	dd if=$< of=$@ bs=512 seek=144

$(dir_out)/Cakes.firm: $(dir_out)/Cakes.bin $(dir_build)/hacks/b9s_screens.bin
	@mkdir -p "$(@D)"
	firmtool build $@ -i -n 0x24000000 -e 0 -D $^ -A 0x23F00000 0x24000000 -C NDMA NDMA

$(dir_out)/Cakes.bin: $(dir_build)/main.elf
	$(OC) -S -O binary $< $@

# Simple make rule for armips cakes.
$(dir_out)/cakes/patches/%.cake: $(dir_patches)/%/recipe.yaml $(dir_patches)/%/patches.s
	@mkdir -p "$(@D)"
	@mkdir -p "$(dir_build)/patches/$*"
	@echo "bake $@"
	@cd $(dir_build)/patches/$* && \
		$(ARMIPS) $(abspath $(dir_patches)/$*/patches.s) && \
		$(PYTHON) $(abspath $(dir_patches)/patissier.py) $(abspath $<) $(abspath $@)

.PHONY: $(dir_out)/3ds/Cakes/Cakes.3dsx $(dir_out)/3ds/Cakes/Cakes.smdh
$(dir_out)/3ds/Cakes/Cakes.3dsx $(dir_out)/3ds/Cakes/Cakes.smdh:
	@mkdir -p "$(@D)"
	@$(MAKE) $(BRAHFLAGS) -C $(dir_cakebrah) all

$(dir_build)/hacks/b9s_screens.bin: $(dir_hacks)/b9s_screens.s
	@mkdir -p "$(@D)"
	@echo "Hack: b9s_screens"
	@cd $(dir_build)/hacks && $(ARMIPS) $(abspath $<)

$(dir_build)/main.elf: $(objects_cfw)
	$(LINK.o) -T linker.ld  $(OUTPUT_OPTION) $^

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

$(dir_build)/%.o: $(dir_source)/%.mono
	@mkdir -p "$(@D)"
	printf '.global $*\n$*: .incbin "$<"\n' | $(COMPILE.s) $(OUTPUT_OPTION)

$(dir_source)/%.mono: | $(dir_source)/%.pbm
	@mkdir -p "$(@D)"
	$(CONVERT) $| $@

include $(call rwildcard, $(dir_build), *.d)
