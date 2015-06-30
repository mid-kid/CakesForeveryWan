#ifndef __patch_h__
#define __patch_h__

#include "fatfs/ff.h"

#define MAX_CAKES 10

struct cake_info {
    char path[_MAX_LFN + 1];
    char description[0x100];
};

struct cake_info *cake_list;
int cake_selected[MAX_CAKES];
int cake_count;

int patch_firm();
int patch_firm_all();
int load_cakes_info(const char *dirpath);

#endif
