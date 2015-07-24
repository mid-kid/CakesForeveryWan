#ifndef __config_h__
#define __config_h__

#include "fatfs/ff.h"

struct config_file {
    unsigned int config_ver;
    unsigned int firm_ver;
    int autoboot_enabled: 1;
    int autoboot_count;
    char autoboot_list[][_MAX_LFN + 1];
} __attribute__((packed));

extern struct config_file *config;
extern int config_modified;

void load_config();
void save_config();

#endif
