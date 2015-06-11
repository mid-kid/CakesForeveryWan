rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

CC := arm-none-eabi-gcc
AS := arm-none-eabi-as
LD := arm-none-eabi-ld
OC := arm-none-eabi-objcopy
PYTHON := python2
OPENSSL := openssl

dir_source := source
dir_build := build
dir_out := out
dir_tools := p3ds

ARM9FLAGS := -mcpu=arm946e-s -march=armv5te
ARM11FLAGS := -mcpu=mpcore
ASFLAGS := -mlittle-endian
CFLAGS := -MMD -MP -marm $(ASFLAGS) -fno-builtin -fshort-wchar -Wall -Wextra -O2 -std=c11 -Wno-main -I $(dir_build)

get_objects = $(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
			  $(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
			  $(call rwildcard, $1, *.s *.c)))

objects_launcher := $(call get_objects, $(dir_source)/launcher)

objects_mset_4x := $(dir_build)/mset_4x/draw.o \
				   $(patsubst $(dir_build)/launcher/%, $(dir_build)/mset_4x/%, \
				   $(objects_launcher) $(objects_mset))
objects_spider_4x := $(patsubst $(dir_build)/launcher/%, $(dir_build)/spider_4x/%, \
					 $(objects_launcher))
objects_spider_5x := $(patsubst $(dir_build)/launcher/%, $(dir_build)/spider_5x/%, \
					 $(objects_launcher))
objects_spider_9x := $(patsubst $(dir_build)/launcher/%, $(dir_build)/spider_9x/%, \
					 $(objects_launcher))

objects_cfw := $(dir_build)/cfw/draw.o $(call get_objects, $(dir_source)/cfw)

rops := $(dir_build)/mset_4x/rop.dat $(dir_build)/spider_4x/rop.dat \
		$(dir_build)/spider_5x/rop.dat $(dir_build)/spider_9x/rop.dat

.PHONY: all
all: $(dir_out)/Launcher.dat

.PHONY: clean
clean:
	rm -rf $(dir_out) $(dir_build)

# Throw everything together
$(dir_out)/Launcher.dat: $(rops) $(dir_build)/cfw/main.bin
	@mkdir -p "$(@D)"
	touch $@
	dd if=$(dir_build)/mset_4x/rop.dat of=$@
	dd if=$(dir_build)/spider_4x/rop.dat of=$@ bs=512 seek=144
	dd if=$(dir_build)/spider_5x/rop.dat of=$@ bs=512 seek=176
	dd if=$(dir_build)/spider_9x/rop.dat of=$@ bs=512 seek=208
	dd if=$(dir_build)/cfw/main.bin of=$@ bs=512 seek=256

$(dir_build)/mset_4x/rop.dat: $(dir_build)/mset_4x/rop.dat.dec
	$(OPENSSL) enc -aes-128-cbc -K 580006192800C5F0FBFB04E06A682088 -iv 00000000000000000000000000000000 -in $< -out $@
$(dir_build)/mset_4x/rop.dat.dec: $(dir_build)/mset_4x/main.bin
	$(PYTHON) $(dir_tools)/build-rop.py MSET_4X $< $@

$(dir_build)/spider_4x/rop.dat: $(dir_build)/spider_4x/rop.dat.dec
	$(PYTHON) $(dir_tools)/spider-encrypt.py $< $@
$(dir_build)/spider_4x/rop.dat.dec: $(dir_build)/spider_4x/main.bin
	$(PYTHON) $(dir_tools)/build-rop.py SPIDER_4X $< $@

$(dir_build)/spider_5x/rop.dat: $(dir_build)/spider_5x/rop.dat.dec
	$(PYTHON) $(dir_tools)/spider-encrypt.py $< $@
$(dir_build)/spider_5x/rop.dat.dec: $(dir_build)/spider_5x/main.bin
	$(PYTHON) $(dir_tools)/build-rop.py SPIDER_5X $< $@

$(dir_build)/spider_9x/rop.dat: $(dir_build)/spider_9x/rop.dat.dec
	$(PYTHON) $(dir_tools)/spider-encrypt.py $< $@
$(dir_build)/spider_9x/rop.dat.dec: $(dir_build)/spider_9x/main.bin
	$(PYTHON) $(dir_tools)/build-rop.py SPIDER_9X $< $@

# Create bin from elf
$(dir_build)/%/main.bin: $(dir_build)/%/main.elf
	$(OC) -S -O binary $< $@

# Different flags for different things
$(dir_build)/cfw/main.elf: ASFLAGS := $(ARM9FLAGS) $(ASFLAGS)
$(dir_build)/cfw/main.elf: CFLAGS := -DARM9 $(ARM9FLAGS) $(CFLAGS)
$(dir_build)/cfw/main.elf: $(objects_cfw)
	# TODO: Undefined reference to '__aeabi_uidiv'
	$(CC) -nostartfiles $(LDFLAGS) -T linker_cfw.ld $(OUTPUT_OPTION) $^
	#$(LD) $(LDFLAGS) -T linker_cfw.ld $(OUTPUT_OPTION) $^

$(dir_build)/mset_4x/main.elf: ASFLAGS := $(ARM11FLAGS) $(ASFLAGS)
$(dir_build)/mset_4x/main.elf: CFLAGS := -DENTRY_MSET -DENTRY_MSET_4x \
							   $(ARM11FLAGS) $(CFLAGS)
$(dir_build)/mset_4x/main.elf: $(objects_mset_4x)
	$(LD) $(LDFLAGS) -T linker_mset.ld $(OUTPUT_OPTION) $^

$(dir_build)/spider_4x/main.elf: ASFLAGS := $(ARM11FLAGS) $(ASFLAGS)
$(dir_build)/spider_4x/main.elf: CFLAGS := -DENTRY_SPIDER -DENTRY_SPIDER_4x \
								 $(ARM11FLAGS) $(CFLAGS)
$(dir_build)/spider_4x/main.elf: $(objects_spider_4x)
	$(LD) $(LDFLAGS) -T linker_spider.ld $(OUTPUT_OPTION) $^

$(dir_build)/spider_5x/main.elf: ASFLAGS := $(ARM11FLAGS) $(ASFLAGS)
$(dir_build)/spider_5x/main.elf: CFLAGS := -DENTRY_SPIDER -DENTRY_SPIDER_5x \
								 $(ARM11FLAGS) $(CFLAGS)
$(dir_build)/spider_5x/main.elf: $(objects_spider_5x)
	$(LD) $(LDFLAGS) -T linker_spider.ld $(OUTPUT_OPTION) $^

$(dir_build)/spider_9x/main.elf: ASFLAGS := $(ARM11FLAGS) $(ASFLAGS)
$(dir_build)/spider_9x/main.elf: CFLAGS := -DENTRY_SPIDER -DENTRY_SPIDER_9x \
								 $(ARM11FLAGS) $(CFLAGS)
$(dir_build)/spider_9x/main.elf: $(objects_spider_9x)
	$(LD) $(LDFLAGS) -T linker_spider.ld $(OUTPUT_OPTION) $^

$(dir_build)/%.o: $(dir_source)/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(dir_build)/%.o: $(dir_source)/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.s) $(OUTPUT_OPTION) $<

$(dir_build)/cfw/fatfs/%.o: $(dir_source)/cfw/fatfs/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) -mthumb -mthumb-interwork $(OUTPUT_OPTION) $<

$(dir_build)/cfw/fatfs/%.o: $(dir_source)/cfw/fatfs/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.s) -mthumb -mthumb-interwork $(OUTPUT_OPTION) $<

.SECONDEXPANSION:
$(dir_build)/%.o: $(dir_source)/launcher/$$(notdir $$*).c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

.SECONDEXPANSION:
$(dir_build)/%.o: $(dir_source)/launcher/$$(notdir $$*).s
	@mkdir -p "$(@D)"
	$(COMPILE.s) $(OUTPUT_OPTION) $<

.SECONDEXPANSION:
$(dir_build)/%.o: $(dir_source)/$$(notdir $$*).c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

include $(call rwildcard, $(dir_build), *.d)
