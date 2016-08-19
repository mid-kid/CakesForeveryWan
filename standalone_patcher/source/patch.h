#pragma once

#include <stdint.h>
#include <stddef.h>
#include "headers.h"

struct memory_header {
    uint32_t location;
    uint32_t size;
};

extern uint32_t *memory_loc;
extern firm_h *firm_loc;
extern firm_h *twl_firm_loc;
extern firm_h *agb_firm_loc;

void patch_reset();
int patch_firm(const void *patch, size_t cake_size);
