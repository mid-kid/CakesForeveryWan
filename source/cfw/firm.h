#ifndef __firm_h__
#define __firm_h__

#include <stdint.h>

#include "headers.h"

typedef struct firm_sig_h
{
    uint8_t sig[0x10];
    uint8_t ver;
} firm_sig_h;

void *firm_loc;
const int firm_size;
int save_firm;
const char *save_path;

int prepare_files();
int decrypt_firm();
int load_firm();
uint8_t get_firm_ver();
void boot_firm();
void boot_cfw();

#endif
