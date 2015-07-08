CC=gcc
LD=ld

TARGET=dhs_client
DHS_DIR=../dhs

CFLAGS=-std=c99 -Wall -Wextra -pedantic -Os -g -I$(DHS_DIR)/include -Wno-padded -Wno-cast-align -Wno-unused-function
# C99 feature test macro for struct addrinfo
CFLAGS+=-D_POSIX_SOURCE
LDFLAGS=

OUT_DIR=obj

OBJS=$(patsubst src/%.c, obj/%.o, $(wildcard src/*.c))
OBJS+=$(patsubst src/%.s, obj/%.o, $(wildcard src/*.s))
OBJS+=$(patsubst src/%.S, obj/%.o, $(wildcard src/*.S))

ifeq ($(OS),Windows_NT)
	TARGET:=$(TARGET).exe
	LDFLAGS+=-lws2_32
endif

.PHONY: clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

obj/%.o: src/%.c | dirs
	@echo Compiling $<
	$(CC) -c $(CFLAGS) $< -o $@

obj/%.o: src/%.s | dirs
	@echo Compiling $<
	$(CC) -c $(CFLAGS) $< -o $@

obj/%.o: src/%.S | dirs
	@echo Compiling $<
	$(CC) -c $(CFLAGS) $< -o $@

dirs: ${OUT_DIR}

${OUT_DIR}:
	mkdir -p ${OUT_DIR}

clean:
	rm -rf obj/* $(TARGET)
