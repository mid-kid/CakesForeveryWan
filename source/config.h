#ifndef __config_h__
#define __config_h__

#include "fatfs/ff.h"

struct config_file {
    int autoboot_count;
    char autoboot_list[][_MAX_LFN + 1];
};

struct config_file *config;

void load_config();
void save_config();

#endif
