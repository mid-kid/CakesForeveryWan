#pragma once

#include <stdint.h>

#include "headers.h"

enum consoles {
    console_o3ds,
    console_n3ds
};

struct firm_signature {
    uint8_t sig[0x10];
    unsigned int version;
    char version_string[8];
    enum consoles console;
};

extern firm_h *firm_loc;
extern struct firm_signature *current_firm;
extern firm_h *agb_firm_loc;
extern struct firm_signature *current_agb_firm;
extern int save_firm;

void slot0x11key96_init();
int load_firm();
int load_firms();
void boot_firm();
void boot_cfw();
