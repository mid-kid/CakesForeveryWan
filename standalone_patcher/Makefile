CFLAGS := -std=c11 -O2 -Wall -Wextra -DSTANDALONE

dir_source := source
dir_build := build

objects := $(patsubst $(dir_source)/%.c, $(dir_build)/%.o, $(wildcard $(dir_source)/*.c))

name := standalone_patcher

.PHONY: all
all: $(name)

.PHONY: clean
clean:
	rm -rf $(dir_build) $(name)

$(name): $(objects)
	$(LINK.o) $(OUTPUT_OPTION) $^

$(dir_build)/%.o: $(dir_source)/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<
