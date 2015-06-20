#ifndef __firm_h__
#define __firm_h__

#include <stdint.h>

struct firm_section {
    void *offset;
    void *address;
    uint32_t size;
    uint32_t type;
    uint8_t hash[0x20];
};

void *firm_loc;
const int firm_size;
int save_firm;
const char *save_path;

int prepare_files();
int decrypt_firm();
void boot_firm();
void boot_cfw(int patch_level);

#endif
