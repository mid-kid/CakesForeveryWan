#ifndef __firm_h__
#define __firm_h__

#include <stdint.h>

#include "headers.h"

firm_h *firm_loc;
uint8_t firm_ver;
int save_firm;
const char *save_path;

int prepare_files();
int decrypt_firm();
int load_firm();
void boot_firm();
void boot_cfw();

#endif
