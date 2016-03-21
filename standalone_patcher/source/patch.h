#pragma once

#include <stdint.h>

struct memory_header {
    uint32_t location;
    uint32_t size;
};

extern uint32_t *memory_loc;

void patch_reset();
int patch_firm(const void *patch);
