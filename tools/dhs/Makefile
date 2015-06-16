export PATH	:=	$(DEVKITARM)/bin:$(PATH)

CC=arm-none-eabi-gcc
CP=arm-none-eabi-g++
OC=arm-none-eabi-objcopy 
LD=arm-none-eabi-ld
AM=armips

CTRUDIR=../ctrulib/libctru

CFLAGS=-std=gnu99 -Os -g -mword-relocations -fomit-frame-pointer -ffast-math
C9FLAGS=-mcpu=arm946e-s -march=armv5te -mlittle-endian
C11FLAGS=-mcpu=mpcore -mlittle-endian
LDFLAGS=
OCFLAGS=--set-section-flags .bss=alloc,load,contents

DHSOBJS=$(patsubst src/%.c, obj/%.o, $(wildcard src/dhs/*.c))
DHSOBJS+=$(patsubst src/%.s, obj/%.o, $(wildcard src/dhs/*.s))
DHSOBJS+=$(patsubst src/%.S, obj/%.o, $(wildcard src/dhs/*.S))

DHS_PATCH_OBJS=$(patsubst src/%.c, obj/%.o, $(wildcard src/dhs_patch/*.c))
DHS_PATCH_OBJS+=$(patsubst src/%.s, obj/%.o, $(wildcard src/dhs_patch/*.s))
DHS_PATCH_OBJS+=$(patsubst src/%.S, obj/%.o, $(wildcard src/dhs_patch/*.S))

OUT_DIR=bin obj/dhs_patch obj/dhs

.PHONY: clean

all: bin/dhs_patch.bin bin/dhs.bin

$(PATCHER): $(PATCHERSRC)
	gcc -g -Isrc/cfw $< -o $@

bin/dhs.bin: $(DHSOBJS) | dirs
	$(CC) -nostartfiles --specs=dhs.specs $(CFLAGS) -march=armv6k -mfloat-abi=hard $(DHSOBJS) -L$(CTRUDIR)/lib -lctru -o $@.elf
	$(OC) $(OCFLAGS) -O binary $@.elf $@

bin/dhs_patch.bin: $(DHS_PATCH_OBJS) | dirs
	$(CC) -nostartfiles --specs=dhs_patch.specs $(CFLAGS) $(DHS_PATCH_OBJS) -o $@.elf
	$(OC) $(OCFLAGS) -O binary $@.elf $@

obj/%.o: src/%.s | dirs
	@echo Compiling $<
	$(CC) -c $(CFLAGS) $(C9FLAGS) $< -o $@

obj/%.o: src/%.S | dirs
	@echo Compiling $<
	$(CC) -c $(CFLAGS) $(C9FLAGS) $< -o $@

obj/%.11.o: src/%.11.s | dirs
	@echo Compiling $<
	$(CC) -c $(CFLAGS) $(C11FLAGS) -fno-toplevel-reorder $< -o $@

obj/%.11.o: src/%.11.S | dirs
	@echo Compiling $<
	$(CC) -c $(CFLAGS) $(C11FLAGS) -fno-toplevel-reorder $< -o $@
	
obj/dhs_patch/%.o: src/dhs_patch/%.c | dirs
	@echo Compiling $<
	$(CC) -c $(CFLAGS) $(C9FLAGS) -fshort-wchar -Iinclude $< -o $@

obj/dhs_patch/%.11.o: src/dhs_patch/%.11.c | dirs
	@echo Compiling $<
	$(CC) -c $(CFLAGS) $(C11FLAGS) -Iinclude $< -o $@

obj/dhs/%.o: src/dhs/%.c | dirs
	@echo Compiling $<
	$(CC) -c $(CFLAGS) $(C11FLAGS) -mfloat-abi=hard -I$(CTRUDIR)/include -Iinclude $< -o $@

dirs: ${OUT_DIR}

${OUT_DIR}:
	mkdir -p ${OUT_DIR}

clean:
	rm -rf bin/*.elf bin/*.bin obj/*
