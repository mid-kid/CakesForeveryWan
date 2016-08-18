#pragma once

// File to keep track of all the fcram offsets in use.
// It provides an easy overview of all that is used.

extern void *fcram_temp;

// Space between most of the locations
#define FCRAM_SPACING 0x100000

// Start of the space we use
#define FCRAM_START 0x24000000

// firm.c
#define FCRAM_FIRM_ORIG_LOC FCRAM_START
#define FCRAM_TWL_FIRM_ORIG_LOC (FCRAM_START + FCRAM_SPACING)  // Double size
#define FCRAM_AGB_FIRM_ORIG_LOC (FCRAM_START + FCRAM_SPACING * 3)

// patch.c
#define FCRAM_FIRM_LOC (FCRAM_START + FCRAM_SPACING * 4)
#define FCRAM_TWL_FIRM_LOC (FCRAM_START + FCRAM_SPACING * 5)  // Double size
#define FCRAM_AGB_FIRM_LOC (FCRAM_START + FCRAM_SPACING * 7)

// patch.c
#define FCRAM_MEMORY_LOC (FCRAM_START + FCRAM_SPACING * 8)
#define FCRAM_FIRM_PATCH_TEMP (FCRAM_START + FCRAM_SPACING * 9)  // Double size (Yes, some sysmodules are bigger than NATIVE_FIRM)
#define FCRAM_CAKE_LIST (FCRAM_START + FCRAM_SPACING * 11)

// config.c
#define FCRAM_CONFIG (FCRAM_START + FCRAM_SPACING * 12)
