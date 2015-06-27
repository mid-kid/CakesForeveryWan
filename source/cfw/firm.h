#ifndef __firm_h__
#define __firm_h__

#include <stdint.h>

#include "headers.h"

void *firm_loc;
const int firm_size;
int save_firm;
const char *save_path;

int prepare_files();
int decrypt_firm();
void boot_firm();
void boot_cfw();

#endif
