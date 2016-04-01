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
static uint32_t firm_size = FCRAM_SPACING;
struct firm_signature *current_firm = NULL;

firm_h *agb_firm_loc = (firm_h *)FCRAM_AGB_FIRM_LOC;
static uint32_t agb_firm_size = FCRAM_SPACING;
struct firm_signature *current_agb_firm = NULL;

static int update_96_keys = 0;
int save_firm = 0;

volatile uint32_t *const a11_entry = (volatile uint32_t *)0x1FFFFFF8;

// We use the firm's section 0's hash to identify the version
struct firm_signature firm_signatures[] = {
    {
        .sig = {0xEE, 0xE2, 0x81, 0x2E, 0xB9, 0x10, 0x0D, 0x03, 0xFE, 0xA2, 0x3F, 0x44, 0xB5, 0x1C, 0xB3, 0x5E},
        .version = 0x1F,
        .version_string = "4.1.0",
        .console = console_o3ds
    }, {
        .sig = {0x8C, 0x29, 0xDA, 0x7B, 0xB5, 0x5F, 0xFE, 0x44, 0x1F, 0x66, 0x79, 0x70, 0x8E, 0xE4, 0x42, 0xE3},
        .version = 0x2A,
        .version_string = "6.1.0",
        .console = console_o3ds
    }, {
        .sig = {0x1D, 0x96, 0x80, 0xD9, 0x0A, 0xA9, 0xDB, 0xE8, 0x29, 0x77, 0xCB, 0x7D, 0x90, 0x55, 0xB7, 0xF9},
        .version = 0x30,
        .version_string = "7.2.0",
        .console = console_o3ds
    }, {
        .sig = {0x3B, 0x61, 0x2E, 0xBA, 0x42, 0xAE, 0x24, 0x46, 0xAD, 0x60, 0x2F, 0x7B, 0x52, 0x16, 0x82, 0x91},
        .version = 0x37,
        .version_string = "8.0.0",
        .console = console_o3ds
    }, {
        .sig = {0x3F, 0xBF, 0x14, 0x06, 0x33, 0x77, 0x82, 0xDE, 0xB2, 0x68, 0x83, 0x01, 0x6B, 0x1A, 0x71, 0x69},
        .version = 0x38,
        .version_string = "9.0.0",
        .console = console_o3ds
    }, {
        .sig = {0x5C, 0x6A, 0x51, 0xF3, 0x79, 0x4D, 0x21, 0x91, 0x0B, 0xBB, 0xFD, 0x17, 0x7B, 0x72, 0x6B, 0x59},
        .version = 0x49,
        .version_string = "9.6.0",
        .console = console_o3ds
    }, {
        .sig = {0xF5, 0x7E, 0xC3, 0x86, 0x1F, 0x8D, 0x8E, 0xFB, 0x44, 0x61, 0xF3, 0x16, 0x51, 0x0A, 0x57, 0x7D},
        .version = 0x50,
        .version_string = "10.4.0",
        .console = console_o3ds
    }, {
        .sig = {0x31, 0xCC, 0x46, 0xCD, 0x61, 0x7A, 0xE7, 0x13, 0x7F, 0xE5, 0xFC, 0x20, 0x46, 0x91, 0x6A, 0xBB},
        .version = 0x04,
        .version_string = "9.0.0",
        .console = console_n3ds
    }, {
        .sig = {0x40, 0x35, 0x6C, 0x9A, 0x24, 0x36, 0x93, 0x7B, 0x76, 0xFE, 0x5D, 0xB1, 0x4D, 0x05, 0x06, 0x52},
        .version = 0x0F,
        .version_string = "9.5.0",
        .console = console_n3ds
    }, {
        .sig = {0x07, 0xFE, 0x9A, 0x62, 0x3F, 0xDE, 0x54, 0xC1, 0x9B, 0x06, 0x91, 0xD8, 0x4F, 0x44, 0x9C, 0x21},
        .version = 0x1B,
        .version_string = "10.2.0",
        .console = console_n3ds
    }, {
        .sig = {0x1A, 0x56, 0x5C, 0xFF, 0xC9, 0xCC, 0x62, 0xBB, 0x2B, 0xC2, 0x23, 0xB6, 0x4F, 0x48, 0xD1, 0xCC},
        .version = 0x1F,
        .version_string = "10.4.0",
        .console = console_n3ds
    }, {.version = 0xFF}
};

struct firm_signature agb_firm_signatures[] = {
    {
        .sig = {0x65, 0xB7, 0x55, 0x78, 0x97, 0xE6, 0x5C, 0xD6, 0x11, 0x74, 0x95, 0xDD, 0x61, 0xE8, 0x08, 0x40},
        .version = 0x0B,
        .version_string = "6.0.0",
        .console = console_o3ds
    }, {.version = 0xFF}
};

void slot0x11key96_init()
{
    // 9.6 crypto may need us to get the key from somewhere else.
    // Unless the console already has the key initialized, that is.
    uint8_t key[AES_BLOCK_SIZE];
    if (read_file(key, PATH_SLOT0X11KEY96, AES_BLOCK_SIZE) == 0) {
        // If we can't read the key, we assume it's not needed, and the firmware is the right version.
        // Otherwise, we make sure the error message for decrypting arm9bin mentions this.
        aes_setkey(0x11, key, AES_KEYNORMAL, AES_INPUT_BE | AES_INPUT_NORMAL);

        // Tell boot_firm it needs to regenerate the keys.
        update_96_keys = 1;
    }
}

int decrypt_cetk_key(void *key, const void *cetk)
{
    // This function only decrypts the NATIVE_FIRM CETK.
    // I don't need it for anything else atm.
    // Either way, this is the reason for the two checks here at the top.

    static int common_key_y_init = 0;
    uint8_t iv[AES_BLOCK_SIZE] = {0};

    uint32_t sigtype = __builtin_bswap32(*(uint32_t *)cetk);
    if (sigtype != SIG_TYPE_RSA2048_SHA256) return 1;

    ticket_h *ticket = (ticket_h *)(cetk + sizeof(sigtype) + 0x13C);
    if (ticket->ticketCommonKeyYIndex != 1) return 1;

    if (!common_key_y_init) {
        uint8_t common_key_y[AES_BLOCK_SIZE] = {0};
        uint8_t *p9_base = (uint8_t *)0x08028000;
        uint8_t *i;
        for (i = p9_base + 0x70000 - AES_BLOCK_SIZE; i >= p9_base; i--) {
            if (i[0] == 0xD0 && i[4] == 0x9C && i[8] == 0x32 && i[12] == 0x23) {
                // At i, there's 7 keys with 4 bytes padding between them.
                // We only need the 2nd.
                memcpy(common_key_y, i + AES_BLOCK_SIZE + 4, sizeof(common_key_y));
                print("Found the common key Y");

                break;
            }
        }
        if (i < p9_base) return 1;

        aes_setkey(0x3D, common_key_y, AES_KEYY, AES_INPUT_BE | AES_INPUT_NORMAL);
        common_key_y_init = 1;
    }

    aes_use_keyslot(0x3D);

    memcpy(iv, ticket->titleID, sizeof(ticket->titleID));

    print("Decrypting key");
    memcpy(key, ticket->titleKey, sizeof(ticket->titleKey));
    aes(key, key, 1, iv, AES_CBC_DECRYPT_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);

    return 0;
}

int decrypt_firm_title(firm_h *dest, ncch_h *ncch, uint32_t *size, void *key)
{
    uint8_t firm_iv[16] = {0};
    uint8_t exefs_key[16] = {0};
    uint8_t exefs_iv[16] = {0};

    print("Decrypting the NCCH");
    aes_setkey(0x16, key, AES_KEYNORMAL, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_use_keyslot(0x16);
    aes(ncch, ncch, *size / AES_BLOCK_SIZE, firm_iv, AES_CBC_DECRYPT_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);

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
    *size = exefs->fileHeaders[0].size;

    if (firm->magic != FIRM_MAGIC) return 1;

    memcpy32(dest, firm, *size);

    return 0;
}

int decrypt_arm9bin(arm9bin_h *header, const unsigned int version)
{
    uint8_t decrypted_keyx[AES_BLOCK_SIZE];

    print("Decrypting ARM9 FIRM binary");

    if (version > 0x0F) {
        slot0x11key96_init();
    }

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

int load_firm(firm_h *dest, char *path, char *path_firmkey, char *path_cetk, uint32_t *size, struct firm_signature *signatures, struct firm_signature **current, int is_native)
{
    uint8_t firm_key[AES_BLOCK_SIZE];
    struct firm_signature *firm_current = NULL;

    if (read_file(dest, path, *size) != 0) {
        print("Failed to load FIRM");

        // Only whine about this if it's NATIVE_FIRM, which is important.
        if (is_native) {
            draw_loading("Failed to load FIRM", "Make sure the encrypted FIRM is\n  located at " PATH_FIRMWARE);
        }
        return 2;
    }
    print("Loaded FIRM");

    if (read_file(firm_key, path_firmkey, AES_BLOCK_SIZE) != 0) {
        print("Failed to load FIRM key,\n  will try to create it...");

        if (read_file(fcram_temp, path_cetk, FCRAM_SPACING) != 0) {
            print("Failed to load CETK");

            if (is_native) {
                draw_loading("Failed to load FIRM key or CETK",
                        "Make sure you have a firmkey.bin or cetk\n"
                        "  located at " PATH_FIRMKEY "\n"
                        "  or " PATH_CETK ", respectively.");
            }
            return 2;
        }
        print("Loaded CETK");

        if (decrypt_cetk_key(firm_key, fcram_temp) != 0) {
            print("Failed to decrypt the CETK");
            draw_loading("Failed to decrypt the CETK", "Please make sure the CETK is right.");
            return 1;
        }
        print("Saving FIRM key for future use");
        write_file(firm_key, path_firmkey, AES_BLOCK_SIZE);
    } else {
        print("Loaded FIRM key");
    }

    print("Decrypting FIRM");
    if (decrypt_firm_title(dest, (void *)dest, size, firm_key) != 0) {
        print("Failed to decrypt the firmware");
        draw_loading("Failed to decrypt the firmware",
                "Please double check your firmware and\n"
                "  firmkey/cetk are right.");
        return 1;
    }

    // Determine firmware version
    for (int i = 0; signatures[i].version != 0xFF; i++) {
        if (memcmp(signatures[i].sig, dest->section[0].hash, 0x10) == 0) {
            firm_current = &signatures[i];
            break;
        }
    }

    if (!firm_current) {
        print("Couldn't determine firmware version");
        draw_loading("Couldn't determine firmware version",
                "The firmware you're trying to use is\n"
                "  most probably not supported by Cakes.\n"
                "Dumping it to your SD card:\n"
                "  " PATH_UNSUPPORTED_FIRMWARE);
        write_file(dest, PATH_UNSUPPORTED_FIRMWARE, firm_size);
        print("Dumped unsupported firmware");
        return 1;
    }

    // The N3DS firm has an additional encryption layer for ARM9
    if (is_native && firm_current->console == console_n3ds) {
        // All the firmwares we've encountered have ARM9 as their second section
        if (decrypt_arm9bin((arm9bin_h *)((uintptr_t)dest + dest->section[2].offset),
                    firm_current->version) != 0) {
            print("Couldn't decrypt ARM9 FIRM binary");
            draw_loading("Couldn't decrypt ARM9 FIRM binary",
                    "Double-check you've got the right firmware.bin.\n"
                    "If you are trying to decrypt a >=9.6 firmware on a <9.6 console, please double-check your key is saved at:\n"
                    "  " PATH_SLOT0X11KEY96 "\n"
                    "We remind you that you can't decrypt it on an old 3ds.\nIf the issue persists, please file a bug report.");
            return 1;
        }
    }

    *current = firm_current;

    return 0;
}

void __attribute__((naked)) disable_lcds()
{
    *a11_entry = 0;  // Don't wait for us

    *(volatile uint32_t *)0x10202A44 = 0;
    *(volatile uint32_t *)0x10202244 = 0;
    *(volatile uint32_t *)0x1020200C = 0;
    *(volatile uint32_t *)0x10202014 = 0;

    while (!*a11_entry);
    ((void (*)())*a11_entry)();
}

void boot_firm()
{
    print("Booting FIRM...");

    // Set up the keys needed to boot a few firmwares, due to them being unset, depending on which firmware you're booting from.
    // TODO: Don't use the hardcoded offset.
    if (update_96_keys && current_firm->console == console_n3ds && (current_firm->version == 0x1B || current_firm->version == 0x1F)) {
        void *keydata = (void *)((uintptr_t)firm_loc + firm_loc->section[2].offset + 0x89814);

        aes_use_keyslot(0x11);
        uint8_t keyx[AES_BLOCK_SIZE];
        for (int slot = 0x19; slot < 0x20; slot++) {
            aes(keyx, keydata, 1, NULL, AES_ECB_DECRYPT_MODE, 0);
            aes_setkey(slot, keyx, AES_KEYX, AES_INPUT_BE | AES_INPUT_NORMAL);
            *(uint8_t *)(keydata + 0xF) += 1;
        }

        print("Updated keyX keyslots");
    }

    firm_section_h *sections = firm_loc->section;

    memcpy32(sections[0].address, (void *)firm_loc + sections[0].offset, sections[0].size);
    memcpy32(sections[1].address, (void *)firm_loc + sections[1].offset, sections[1].size);
    memcpy32(sections[2].address, (void *)firm_loc + sections[2].offset, sections[2].size);
    print("Copied FIRM");

    *a11_entry = (uint32_t)disable_lcds;
    while (*a11_entry);  // Make sure it jumped there correctly before changing it.
    *a11_entry = (uint32_t)firm_loc->a11Entry;
    print("Prepared arm11 entry");

    print("Booting...");

    if (current_firm->console == console_n3ds) {
        // Jump to the actual entry instead of the loader
        ((void (*)())0x0801B01C)();
    } else {
        ((void (*)())firm_loc->a9Entry)();
    }
}

int load_firms()
{
    const char *title = "Loading firm";

    print("Loading NATIVE_FIRM...");
    draw_loading(title, "Loading NATIVE_FIRM...");
    if (load_firm(firm_loc, PATH_FIRMWARE, PATH_FIRMKEY, PATH_CETK, &firm_size, firm_signatures, &current_firm, 1) != 0) return 1;

    print("Loading AGB_FIRM...");
    draw_loading(title, "Loading AGB_FIRM...");
    if (load_firm(agb_firm_loc, PATH_AGB_FIRMWARE, PATH_AGB_FIRMKEY, PATH_AGB_CETK, &agb_firm_size, agb_firm_signatures, &current_agb_firm, 0) == 1) return 1;

    return 0;
}

void boot_cfw()
{
    const char *title = "Booting CFW";

    draw_loading(title, "Patching...");
    if (patch_firm_all() != 0) return;

    // Only save the firm if that option is required (or it's needed for autoboot),
    //   and either the patches have been modified, or the file doesn't exist.
    if ((save_firm || config->autoboot_enabled) &&
            (patches_modified || f_stat(PATH_PATCHED_FIRMWARE, NULL) != 0)) {
        draw_loading(title, "Saving NATIVE_FIRM...");
        print("Saving patched NATIVE_FIRM");
        if (write_file(firm_loc, PATH_PATCHED_FIRMWARE, firm_size) != 0) {
            draw_message("Failed to save the patched FIRM",
                    "One or more patches you selected requires this.\n"
                    "But, for some reason, we failed to write it.");
            return;
        }
    }

    if (current_agb_firm && (patches_modified || f_stat(PATH_PATCHED_AGB_FIRMWARE, NULL) != 0)) {
        draw_loading(title, "Saving AGB_FIRM...");
        print("Saving patched AGB_FIRM");
        if (write_file(agb_firm_loc, PATH_PATCHED_AGB_FIRMWARE, agb_firm_size) != 0) {
            draw_message("Failed to save the patched FIRM", "For some reason, we haven't been able to write to the SD card.");
            return;
        }
    }

    draw_loading(title, "Booting...");
    boot_firm();
}
