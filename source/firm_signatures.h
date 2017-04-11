#pragma once

#include <stdint.h>

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
