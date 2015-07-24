#ifndef __firm_h__
#define __firm_h__

#include <stdint.h>

#include "headers.h"

extern firm_h *firm_loc;
extern unsigned int firm_ver;
extern int save_firm;
extern const char *save_path;

int prepare_files();
int decrypt_firm();
int load_firm();
void boot_firm();
void boot_cfw();

#endif
