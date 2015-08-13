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

struct arm9bin_h {
    uint8_t keyx[0x10];
    uint8_t keyy[0x10];
    uint8_t ctr[0x10];
    char size[8];
    uint8_t pad[8];
    uint8_t ctl_block[0x10];
    uint8_t unk[0x10];
    uint8_t slot0x16keyX[0x10];
};

extern firm_h *firm_loc;
extern struct firm_signature *current_firm;
extern int save_firm;
extern const char *save_path;

int prepare_files();
int decrypt_firm();
int load_firm();
void boot_firm();
void boot_cfw();

#endif
