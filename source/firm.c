#include "firm.h"

#include <stdint.h>
#include <stddef.h>
#include "headers.h"
#include "draw.h"
#include "memfuncs.h"
#include "fs.h"
#include "menu.h"
#include "crypto.h"
#include "patch.h"
#include "config.h"
#include "fcram.h"
#include "paths.h"
#include "fatfs/ff.h"

firm_h *firm_loc = (firm_h *)FCRAM_FIRM_LOC;
static int firm_size = 0;
static void *firm_loc_encrypted = (void *)FCRAM_FIRM_LOC_ENCRYPTED;
static const int firm_size_encrypted = FCRAM_SPACING;

static int firm_key_loaded = 0;
static void *firm_key_cetk = (void *)FCRAM_FIRM_KEY_CETK;
static uint8_t firm_key[AES_BLOCK_SIZE];

struct firm_signature *current_firm = NULL;
int save_firm = 0;

// We use the firm's section 0's hash to identify the version
struct firm_signature firm_signatures[] = {
    {
        .sig = {0xEE, 0xE2, 0x81, 0x2E, 0xB9, 0x10, 0x0D, 0x03, 0xFE, 0xA2, 0x3F, 0x44, 0xB5, 0x1C, 0xB3, 0x5E},
        .version = 0x1F,
        .console = console_o3ds
    }, {
        .sig = {0x8C, 0x29, 0xDA, 0x7B, 0xB5, 0x5F, 0xFE, 0x44, 0x1F, 0x66, 0x79, 0x70, 0x8E, 0xE4, 0x42, 0xE3},
        .version = 0x2A,
        .console = console_o3ds
    }, {
        .sig = {0x1D, 0x96, 0x80, 0xD9, 0x0A, 0xA9, 0xDB, 0xE8, 0x29, 0x77, 0xCB, 0x7D, 0x90, 0x55, 0xB7, 0xF9},
        .version = 0x30,
        .console = console_o3ds
    }, {
        .sig = {0x3B, 0x61, 0x2E, 0xBA, 0x42, 0xAE, 0x24, 0x46, 0xAD, 0x60, 0x2F, 0x7B, 0x52, 0x16, 0x82, 0x91},
        .version = 0x37,
        .console = console_o3ds
    }, {
        .sig = {0x3F, 0xBF, 0x14, 0x06, 0x33, 0x77, 0x82, 0xDE, 0xB2, 0x68, 0x83, 0x01, 0x6B, 0x1A, 0x71, 0x69},
        .version = 0x38,
        .console = console_o3ds
    }, {
        .sig = {0x5C, 0x6A, 0x51, 0xF3, 0x79, 0x4D, 0x21, 0x91, 0x0B, 0xBB, 0xFD, 0x17, 0x7B, 0x72, 0x6B, 0x59},
        .version = 0x49,
        .console = console_o3ds
    }, {
        .sig = {0x40, 0x35, 0x6C, 0x9A, 0x24, 0x36, 0x93, 0x7B, 0x76, 0xFE, 0x5D, 0xB1, 0x4D, 0x05, 0x06, 0x52},
        .version = 0x0F,
        .console = console_n3ds
    }, {
        .sig = {0x31, 0xCC, 0x46, 0xCD, 0x61, 0x7A, 0xE7, 0x13, 0x7F, 0xE5, 0xFC, 0x20, 0x46, 0x91, 0x6A, 0xBB},
        .version = 0x04,
        .console = console_n3ds
    }, {.version = 0xFF}
};

int prepare_files()
{
    int rc;

    rc = read_file(firm_loc_encrypted, PATH_FIRMWARE, firm_size_encrypted);
    if (rc != 0) {
        print("Failed to load FIRM");
        draw_loading("Failed to load FIRM", "Make sure the encrypted FIRM is\n  located at " PATH_FIRMWARE);
        return 1;
    }
    print("Loaded FIRM");

    rc = read_file(firm_key, PATH_FIRMKEY, sizeof(firm_key));
    if (rc != 0) {
        print("Failed to load FIRM key,\n  will try to create it...");

        rc = read_file(firm_key_cetk, PATH_CETK, FCRAM_SPACING);
        if (rc != 0) {
            print("Failed to load CETK");
            draw_loading("Failed to load FIRM key or CETK", "Make sure you have a firmkey.bin or cetk\n  located at " PATH_FIRMKEY "\n  or " PATH_CETK ", respectively.");
            return 1;
        }
        print("Loaded CETK");
    } else {
        firm_key_loaded = 1;
        print("Loaded FIRM key");
    }

    return 0;
}

int decrypt_cetk_key(void *key, const void *cetk)
{
    // This function only decrypts the NATIVE_FIRM CETK.
    // I don't need it for anything else atm.
    // Either way, this is the reason for the two checks here at the top.

    uint8_t common_key_y[AES_BLOCK_SIZE];
    uint8_t iv[AES_BLOCK_SIZE] = {0};

    uint32_t sigtype = __builtin_bswap32(*(uint32_t *)cetk);
    if (sigtype != SIG_TYPE_RSA2048_SHA256) return 1;

    ticket_h *ticket = (ticket_h *)(cetk + sizeof(sigtype) + 0x13C);
    if (ticket->ticketCommonKeyYIndex != 1) return 1;

    uint8_t *p9_base = (uint8_t *)0x08028000;
    uint8_t *i;
    for (i = p9_base + 0x6C000; i < p9_base + 0x6C000 + 0x4000; i++) {
        if (i[0] == 0xD0 && i[4] == 0x9C && i[8] == 0x32 && i[12] == 0x23) {

            // At i, there's 7 keys with 4 bytes padding between them.
            // We only need the 2nd.
            memcpy(common_key_y, i + AES_BLOCK_SIZE + 4, sizeof(common_key_y));
            print("Found the common key Y");

            break;
        }
    }
    if (i >= p9_base + 0x6C000 + 0x4000) return 1;

    aes_setkey(0x3D, common_key_y, AES_KEYY, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_use_keyslot(0x3D);

    memcpy(iv, ticket->titleID, sizeof(ticket->titleID));

    print("Decrypting key");
    memcpy(key, ticket->titleKey, sizeof(ticket->titleKey));
    aes(key, key, 1, iv, AES_CBC_DECRYPT_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);

    return 0;
}

int decrypt_firm_title(firm_h *dest, ncch_h *ncch, const uint32_t size)
{
    uint8_t firm_iv[16] = {0};
    uint8_t exefs_key[16] = {0};
    uint8_t exefs_iv[16] = {0};

    print("Decrypting the NCCH");
    aes_setkey(0x16, firm_key, AES_KEYNORMAL, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_use_keyslot(0x16);
    aes(ncch, ncch, size / AES_BLOCK_SIZE, firm_iv, AES_CBC_DECRYPT_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);

    if (ncch->magic != NCCH_MAGIC) return 1;

    memcpy(exefs_key, ncch, 16);
    ncch_getctr(ncch, exefs_iv, NCCHTYPE_EXEFS);

    // Get the exefs offset and size from the NCCH
    exefs_h *exefs = (exefs_h *)((void *)ncch + ncch->exeFSOffset * MEDIA_UNITS);
    uint32_t exefs_size = ncch->exeFSSize * MEDIA_UNITS;

    print("Decrypting the exefs");
    aes_setkey(0x2C, exefs_key, AES_KEYY, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_use_keyslot(0x2C);
    aes(exefs, exefs, exefs_size / AES_BLOCK_SIZE, exefs_iv, AES_CTR_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);

    // Get the decrypted FIRM
    // We assume the firm.bin is always the first file
    firm_h *firm = (firm_h *)&exefs[1];  // The offset right behind the exefs header; the first file.
    firm_size = exefs->fileHeaders[0].size;

    if (firm->magic != FIRM_MAGIC) return 1;

    print("Copying the FIRM");
    memcpy32(dest, firm, firm_size);

    return 0;
}


int decrypt_arm9bin(arm9bin_h *header, const unsigned int version) {
    uint8_t decrypted_keyx[16];

    print("Decrypting ARM9 FIRM binary");

    aes_use_keyslot(0x11);
    if (version < 0x0F) {
        aes(decrypted_keyx, header->keyx, 1, NULL, AES_ECB_DECRYPT_MODE, 0);
    } else {
        aes(decrypted_keyx, header->slot0x16keyX, 1, NULL, AES_ECB_DECRYPT_MODE, 0);
    }

    aes_setkey(0x16, decrypted_keyx, AES_KEYX, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_setkey(0x16, header->keyy, AES_KEYY, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_setiv(header->ctr, AES_INPUT_BE | AES_INPUT_NORMAL);

    void *arm9bin = (uint8_t *)header + 0x800;
    int size = atoi(header->size);

    aes_use_keyslot(0x16);
    aes(arm9bin, arm9bin, size / AES_BLOCK_SIZE, header->ctr, AES_CTR_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);

    if (*(uint32_t *)arm9bin != ARM9BIN_MAGIC) return 1;

    return 0;
}

int decrypt_firm()
{
    if (!firm_key_loaded) {
        print("Obtaining FIRM key from CETK");
        if (decrypt_cetk_key(firm_key, firm_key_cetk) != 0) {
            draw_loading("Failed to decrypt the cetk", "Please make sure the cetk is right.");
            return 1;
        }
        firm_key_loaded = 1;
        print("Saving FIRM key for future use");
        write_file(firm_key, PATH_FIRMKEY, sizeof(firm_key));
    }

    print("Decrypting FIRM");
    if (decrypt_firm_title(firm_loc, firm_loc_encrypted, firm_size_encrypted) != 0) {
        print("Failed to decrypt the firmware.bin");
        draw_loading("Failed to decrypt the firmware.bin", "Please double check your firmware.bin and\n  firmkey.bin are right.");
        return 1;
    }

    // Determine firmware version
    for (int i = 0; firm_signatures[i].version != 0xFF; i++) {
        if (memcmp(firm_signatures[i].sig, firm_loc->section[0].hash, 0x10) == 0) {
            current_firm = &firm_signatures[i];
            break;
        }
    }

    if (!current_firm) {
        print("Couldn't determine firmware version");
        draw_loading("Couldn't determine firmware version", "The firmware.bin you're trying to use is\n  most probably not supported by Cakes.");
        return 1;
    }

    // The N3DS firm has an additional encryption layer for ARM9
    if (current_firm->console == console_n3ds) {
        // All the firmwares we've encountered have ARM9 as their second section
        if (decrypt_arm9bin((arm9bin_h *)((uintptr_t)firm_loc + firm_loc->section[2].offset),
                    current_firm->version) != 0) {
            print("Couldn't decrypt ARM9 FIRM binary");
            draw_loading("Coudn't decrypt ARM9 FIRM binary", "Double-check you've got the right firmware.bin.\n  We remind you that you can't decrypt it on an old 3ds.\n  If the issue persists, please file a bug report.");
            return 1;
        }
    }

    return 0;
}

void boot_firm()
{
    print("Booting FIRM...");

    __asm__ (
        "msr cpsr_c, #0xDF\n\t"
        "ldr r0, =0x10000035\n\t"
        "mcr p15, 0, r0, c6, c3, 0\n\t"
        "mrc p15, 0, r0, c2, c0, 0\n\t"
        "mrc p15, 0, r12, c2, c0, 1\n\t"
        "mrc p15, 0, r1, c3, c0, 0\n\t"
        "mrc p15, 0, r2, c5, c0, 2\n\t"
        "mrc p15, 0, r3, c5, c0, 3\n\t"
        "ldr r4, =0x18000035\n\t"
        "bic r2, r2, #0xF0000\n\t"
        "bic r3, r3, #0xF0000\n\t"
        "orr r0, r0, #0x10\n\t"
        "orr r2, r2, #0x30000\n\t"
        "orr r3, r3, #0x30000\n\t"
        "orr r12, r12, #0x10\n\t"
        "orr r1, r1, #0x10\n\t"
        "mcr p15, 0, r0, c2, c0, 0\n\t"
        "mcr p15, 0, r12, c2, c0, 1\n\t"
        "mcr p15, 0, r1, c3, c0, 0\n\t"
        "mcr p15, 0, r2, c5, c0, 2\n\t"
        "mcr p15, 0, r3, c5, c0, 3\n\t"
        "mcr p15, 0, r4, c6, c4, 0\n\t"
        "mrc p15, 0, r0, c2, c0, 0\n\t"
        "mrc p15, 0, r1, c2, c0, 1\n\t"
        "mrc p15, 0, r2, c3, c0, 0\n\t"
        "orr r0, r0, #0x20\n\t"
        "orr r1, r1, #0x20\n\t"
        "orr r2, r2, #0x20\n\t"
        "mcr p15, 0, r0, c2, c0, 0\n\t"
        "mcr p15, 0, r1, c2, c0, 1\n\t"
        "mcr p15, 0, r2, c3, c0, 0\n\t"
        ::: "r0", "r1", "r2", "r3", "r4", "r12"
    );
    print("Set up MPU");

    firm_section_h *sections = firm_loc->section;

    memcpy32(sections[0].address, (void *)firm_loc + sections[0].offset, sections[0].size);
    memcpy32(sections[1].address, (void *)firm_loc + sections[1].offset, sections[1].size);
    memcpy32(sections[2].address, (void *)firm_loc + sections[2].offset, sections[2].size);
    print("Copied FIRM");

    *(uint32_t *)0x1FFFFFF8 = (uint32_t)firm_loc->a11Entry;
    print("Prepared arm11 entry");

    print("Booting...");

    if (current_firm->console == console_n3ds) {
        // Jump to the actual entry instead of the loader
        ((void (*)())0x0801B01C)();
    } else {
        ((void (*)())firm_loc->a9Entry)();
    }
}

int load_firm()
{
    char *title = "Loading firm";

    draw_loading(title, "Loading...");
    if (prepare_files() != 0) return 1;

    draw_loading(title, "Decrypting FIRM...");
    if (decrypt_firm() != 0) return 1;

    return 0;
}

void boot_cfw()
{
    char *title = "Booting CFW";

    draw_loading(title, "Patching...");
    if (patch_firm_all() != 0) return;

    // Only save the firm if that option is required (or it's needed for autoboot),
    //   and either the patches have been modified, or the file doesn't exist.
    if ((save_firm || config->autoboot_enabled) &&
            (patches_modified || f_stat(PATH_PATCHED_FIRMWARE, NULL) != 0)) {
        draw_loading(title, "Saving FIRM...");
        print("Saving patched FIRM");
        if (write_file(firm_loc, PATH_PATCHED_FIRMWARE, firm_size) != 0) {
            draw_message("Failed to save FIRM", "One or more patches you selected requires this.\nBut, for some reason, I failed to write it.");
            return;
        }
    }

    draw_loading(title, "Booting...");
    boot_firm();
}
