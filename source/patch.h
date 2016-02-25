#pragma once

#include <stdint.h>
#include "fatfs/ff.h"

#define MAX_CAKES 10

struct cake_info {
    char path[_MAX_LFN + 1];
    char description[0x100];
};

extern struct cake_info *cake_list;
extern unsigned int cake_count;
int cake_selected[MAX_CAKES];

int get_emunand_offsets(uint32_t location, uint32_t *offset, uint32_t *header);
int patch_firm_all();
int load_cakes_info(const char *dirpath);
