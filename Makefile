rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

CC := arm-none-eabi-gcc
AS := arm-none-eabi-as
LD := arm-none-eabi-ld
OBJCOPY := arm-none-eabi-objcopy
PYTHON := python2
OPENSSL := openssl

define bin2o
	bin2s $< | $(AS) -o $(@)
	echo "#include <stdint.h>" > `(echo $(@D)/$(<F) | tr . _)`.h
	echo "extern uint8_t" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" >> `(echo $(@D)/$(<F) | tr . _)`.h
	echo "extern uint8_t" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(@D)/$(<F) | tr . _)`.h
	echo "extern uint32_t" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(@D)/$(<F) | tr . _)`.h
endef

dir_source := source
dir_build := build
dir_out := out
dir_tools := p3ds

ASFLAGS := -mcpu=mpcore -mlittle-endian
CFLAGS := -marm $(ASFLAGS) -mword-relocations -fno-builtin -fshort-wchar -Wall -Wextra -O2 -std=c11 -Wno-main -I $(dir_build)
LDFLAGS := -T linker.ld

sources := $(call rwildcard, $(dir_source)/, *.c *.s)
objects := $(dir_build)/arm9hax.bin.o $(dir_build)/cfw.bin.o $(patsubst $(dir_source)/%.s, $(dir_build)/%.o, $(patsubst $(dir_source)/%.c, $(dir_build)/%.o, $(sources)))

.PHONY: all
all: $(dir_out)/Launcher.dat

.PHONY: clean
clean:
	rm -rf $(dir_out) $(dir_build)

$(dir_out)/Launcher.dat: $(dir_out)/MsetForBoss.dat
	$(OPENSSL) enc -aes-128-cbc -K 580006192800C5F0FBFB04E06A682088 -iv 00000000000000000000000000000000 -in $< -out $@

$(dir_out)/MsetForBoss.dat: $(dir_build)/main.bin
	@mkdir -p "$(@D)"
	#$(PYTHON) $(dir_tools)/gspwn-rop.py $< $@
	armips rop.s

$(dir_build)/main.bin: $(dir_build)/main.elf
	$(OBJCOPY) -S -O binary $< $@

$(dir_build)/main.elf: $(objects)
	$(LD) $(LDFLAGS) $(OUTPUT_OPTION) $^

$(dir_build)/%.bin.o: %.bin
	@mkdir -p "$(@D)"
	@echo "Creating $@"
	@$(bin2o)

$(dir_build)/%.o: $(dir_source)/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(dir_build)/%.o: $(dir_source)/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.s) $(OUTPUT_OPTION) $<
