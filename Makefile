CC := arm-none-eabi-gcc
AS := arm-none-eabi-as
LD := arm-none-eabi-ld
OC := arm-none-eabi-objcopy
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

objects_mset := $(dir_build)/launcher/draw.o
objects_launcher := $(filter-out $(objects_mset), \
					$(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
					$(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
					$(wildcard $(dir_source)/launcher/*.s $(dir_source)/launcher/*.c))))

objects_mset_4x := $(patsubst $(dir_build)/launcher/%, $(dir_build)/mset_4x/%, \
				   $(objects_launcher) $(objects_mset))
objects_spider_4x := $(patsubst $(dir_build)/launcher/%, $(dir_build)/spider_4x/%, \
					 $(objects_launcher))
objects_spider_5x := $(patsubst $(dir_build)/launcher/%, $(dir_build)/spider_5x/%, \
					 $(objects_launcher))
objects_spider_9x := $(patsubst $(dir_build)/launcher/%, $(dir_build)/spider_9x/%, \
					 $(objects_launcher))

rops := $(dir_build)/mset_4x/rop.dat $(dir_build)/spider_4x/rop.dat \
		$(dir_build)/spider_5x/rop.dat $(dir_build)/spider_9x/rop.dat

.PHONY: all
all: $(dir_out)/Launcher.dat

.PHONY: clean
clean:
	rm -rf $(dir_out) $(dir_build)

# Throw everything together
$(dir_out)/Launcher.dat: $(rops)
	mkdir -p "$(@D)"
	touch $@
	dd if=$(dir_build)/mset_4x/rop.dat of=$@
	dd if=$(dir_build)/spider_4x/rop.dat of=$@ bs=512 seek=144
	dd if=$(dir_build)/spider_5x/rop.dat of=$@ bs=512 seek=176
	dd if=$(dir_build)/spider_9x/rop.dat of=$@ bs=512 seek=208
	dd if=cfw.bin of=$@ bs=512 seek=256

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

# Different flags for different entry points
$(dir_build)/mset_4x/main.elf: CFLAGS := -DENTRY_MSET -DENTRY_MSET_4x $(CFLAGS)
$(dir_build)/mset_4x/main.elf: $(objects_mset_4x)
	$(LD) $(LDFLAGS) -T linker_mset.ld $(OUTPUT_OPTION) $^

$(dir_build)/spider_4x/main.elf: CFLAGS := -DENTRY_SPIDER -DENTRY_SPIDER_4x $(CFLAGS)
$(dir_build)/spider_4x/main.elf: $(objects_spider_4x)
	$(LD) $(LDFLAGS) -T linker_spider.ld $(OUTPUT_OPTION) $^

$(dir_build)/spider_5x/main.elf: CFLAGS := -DENTRY_SPIDER -DENTRY_SPIDER_5x $(CFLAGS)
$(dir_build)/spider_5x/main.elf: $(objects_spider_5x)
	$(LD) $(LDFLAGS) -T linker_spider.ld $(OUTPUT_OPTION) $^

$(dir_build)/spider_9x/main.elf: CFLAGS := -DENTRY_SPIDER -DENTRY_SPIDER_9x $(CFLAGS)
$(dir_build)/spider_9x/main.elf: $(objects_spider_9x)
	$(LD) $(LDFLAGS) -T linker_spider.ld $(OUTPUT_OPTION) $^

$(dir_build)/%.bin.o: %.bin
	@mkdir -p "$(@D)"
	@echo "Creating $@"
	@$(bin2o)

.SECONDEXPANSION:
$(dir_build)/%.o: $(dir_source)/launcher/$$(notdir $$*).c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

.SECONDEXPANSION:
$(dir_build)/%.o: $(dir_source)/launcher/$$(notdir $$*).s
	@mkdir -p "$(@D)"
	$(COMPILE.s) $(OUTPUT_OPTION) $<
