#pragma once

#include <stdint.h>
#include "fatfs/ffconf.h"

struct config_file {
    unsigned int config_ver;
    unsigned int firm_ver;
    uint8_t firm_console;
    uint32_t emunand_location;
    unsigned int autoboot_enabled: 1;
    unsigned int cake_count;
    char cake_list[][_MAX_LFN + 1];
} __attribute__((packed));

extern struct config_file *config;
extern int patches_modified;

void load_config();
void load_config_cakes();
void save_config();
