#pragma once

#include <stdint.h>

#include "headers.h"

enum consoles {
    console_o3ds,
    console_n3ds
};

enum firm_types {
    NATIVE_FIRM,
    TWL_FIRM,
    AGB_FIRM
};

struct firm_signature {
    uint8_t sig[0x10];
    unsigned int version;
    char version_string[8];
    enum consoles console;
};

extern struct firm_signature firm_signatures[];
extern struct firm_signature twl_firm_signatures[];
extern struct firm_signature agb_firm_signatures[];
extern firm_h *firm_loc;
extern struct firm_signature *current_firm;
extern firm_h *twl_firm_loc;
extern struct firm_signature *current_twl_firm;
extern firm_h *agb_firm_loc;
extern struct firm_signature *current_agb_firm;
extern int save_firm;

struct firm_signature *get_firm_info(firm_h *firm, struct firm_signature *signatures);
void slot0x11key96_init();
int load_firms();
void boot_firm();
void boot_cfw();
