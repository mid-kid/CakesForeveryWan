#ifndef __firm_h__
#define __firm_h__

#include <stdint.h>

#include "headers.h"

enum consoles {
    console_o3ds,
    console_n3ds
};

struct firm_signature {
    uint8_t sig[0x10];
    unsigned int version;
    enum consoles console;
};

extern firm_h *firm_loc;
extern struct firm_signature *current_firm;
extern int save_firm;

int prepare_files();
int decrypt_firm();
int load_firm();
void boot_firm();
void boot_cfw();

#endif
