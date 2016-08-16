#include "firm.h"

#ifndef STANDALONE
#include <stdint.h>
#include <stddef.h>
#include "headers.h"
#include "draw.h"
#include "memfuncs.h"
#include "fs.h"
#include "menu.h"
#include "patch.h"
#include "config.h"
#include "fcram.h"
#include "paths.h"
#include "fatfs/ff.h"
#include "fatfs/sdmmc/sdmmc.h"
#include "external/crypto.h"
#else
#include <string.h>
#endif

#ifndef STANDALONE
firm_h *firm_orig_loc = (firm_h *)FCRAM_FIRM_ORIG_LOC;
size_t firm_size = FCRAM_SPACING;
struct firm_signature *current_firm = NULL;

firm_h *twl_firm_orig_loc = (firm_h *)FCRAM_TWL_FIRM_ORIG_LOC;
size_t twl_firm_size = FCRAM_SPACING * 2;
struct firm_signature *current_twl_firm = NULL;

firm_h *agb_firm_orig_loc = (firm_h *)FCRAM_AGB_FIRM_ORIG_LOC;
size_t agb_firm_size = FCRAM_SPACING;
struct firm_signature *current_agb_firm = NULL;

static int update_96_keys = 0;
int save_firm = 0;
#endif

#define A9LHBOOT (*(volatile uint8_t *)0x10010000 == 0) // CFG_BOOTENV
static volatile uint32_t *const a11_entry = (volatile uint32_t *)0x1FFFFFF8;

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
        .sig = {0xE9, 0xAD, 0x74, 0x9D, 0x46, 0x9C, 0x9C, 0xF4, 0x96, 0x9E, 0x1A, 0x7A, 0xDF, 0x40, 0x2A, 0x82},
        .version = 0x52,
        .version_string = "11.0.0",
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
    }, {
        .sig = {0x52, 0x30, 0x0F, 0x55, 0xA2, 0x64, 0x4E, 0xFF, 0x96, 0x90, 0xF0, 0xE5, 0x6E, 0xC8, 0x2E, 0xB3},
        .version = 0x21,
        .version_string = "11.0.0",
        .console = console_n3ds
    }, {.version = 0xFF}
};

struct firm_signature twl_firm_signatures[] = {
    {
        .sig = {0xE8, 0xB8, 0x82, 0xF5, 0x8C, 0xC4, 0x1B, 0x24, 0x05, 0x60, 0x6D, 0xB8, 0x74, 0xF5, 0xE5, 0xDD},
        .version = 0x16,
        .version_string = "6.2.0",
        .console = console_o3ds
    }, {
        .sig = {0x0F, 0x05, 0xC5, 0xF3, 0x60, 0x83, 0x8B, 0x9D, 0xC8, 0x44, 0x3F, 0xB3, 0x06, 0x4D, 0x30, 0xC7},
        .version = 0x00,
        .version_string = "9.0.0",
        .console = console_n3ds
    }, {.version = 0xFF}
};

struct firm_signature agb_firm_signatures[] = {
    {
        .sig = {0x65, 0xB7, 0x55, 0x78, 0x97, 0xE6, 0x5C, 0xD6, 0x11, 0x74, 0x95, 0xDD, 0x61, 0xE8, 0x08, 0x40},
        .version = 0x0B,
        .version_string = "6.0.0",
        .console = console_o3ds
    }, {
        .sig = {0xAF, 0x81, 0xA1, 0xAB, 0xBA, 0xAC, 0xAC, 0xA7, 0x30, 0xE8, 0xD8, 0x74, 0x7C, 0x47, 0x1C, 0x5D},
        .version = 0x00,
        .version_string = "9.0.0",
        .console = console_n3ds
    }, {.version = 0xFF}
};

struct firm_signature *get_firm_info(firm_h *firm, struct firm_signature *signatures)
{
    for (struct firm_signature *signature = signatures; signature->version != 0xFF; signature++) {
        if (memcmp(signature->sig, firm->section[0].hash, 0x10) == 0) {
            return signature;
        }
    }

    return NULL;
}

#ifndef STANDALONE
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

int dump_firm(void *firm_buffer, const uint8_t firm_id)
{
    // 0x0B130000 = start of FIRM0 partition, 0x400000 = size of FIRM partition (4MB)
    uint32_t firm_offset = (0x0B130000 + ((firm_id % 2) * 0x400000)),
             firm_size = 0x100000; // 1MB, because that's the current FIRM size

    uint8_t nand_ctr[0x10],
            nand_cid[0x10],
            sha_hash[0x20];

    if (sdmmc_nand_readsectors(firm_offset / 0x200, firm_size / 0x200, firm_buffer) != 0) return -1;

    sdmmc_get_cid(1, (uint32_t*) nand_cid);
    sha(sha_hash, nand_cid, 0x10, SHA_256_MODE);
    memcpy(nand_ctr, sha_hash, 0x10);
    aes_advctr(nand_ctr, firm_offset / AES_BLOCK_SIZE, AES_INPUT_BE | AES_INPUT_NORMAL);

    aes_use_keyslot(0x06);
    aes_setiv(nand_ctr, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes(firm_buffer, firm_buffer, firm_size / AES_BLOCK_SIZE, nand_ctr, AES_CTR_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);

    return 0;
}

int decrypt_arm9bin(arm9bin_h *header, enum firm_types firm_type, const unsigned int version)
{
    uint8_t slot = 0x15;

    print("Decrypting ARM9 FIRM binary");

    if (firm_type == NATIVE_FIRM && version > 0x0F) {
        uint8_t decrypted_keyx[AES_BLOCK_SIZE];

        slot0x11key96_init();
        slot = 0x16;

        aes_use_keyslot(0x11);
        aes(decrypted_keyx, header->slot0x16keyX, 1, NULL, AES_ECB_DECRYPT_MODE, 0);
        aes_setkey(slot, decrypted_keyx, AES_KEYX, AES_INPUT_BE | AES_INPUT_NORMAL);
    }

    aes_setkey(slot, header->keyy, AES_KEYY, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_setiv(header->ctr, AES_INPUT_BE | AES_INPUT_NORMAL);

    void *arm9bin = (uint8_t *)header + 0x800;
    int size = atoi(header->size);

    aes_use_keyslot(slot);
    aes(arm9bin, arm9bin, size / AES_BLOCK_SIZE, header->ctr, AES_CTR_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);

    if (firm_type == NATIVE_FIRM) return *(uint32_t *)arm9bin != ARM9BIN_MAGIC;
    else if (firm_type == AGB_FIRM || firm_type == TWL_FIRM) return *(uint32_t *)arm9bin != LGY_ARM9BIN_MAGIC;
    else return 0;
}

int decrypt_cetk_key(void *key, const void *cetk)
{
    // This function only decrypts the FIRM CETK.
    // I don't need it for anything else atm.
    // Either way, this is the reason for the two checks here at the top.

    static int common_key_y_init = 0;
    uint8_t iv[AES_BLOCK_SIZE] = {0};
    const uint8_t key_y_hash[0x20] = {0x21, 0x12, 0xF4, 0x50, 0x78, 0x6D, 0xCE, 0x64, 0x39, 0xFD, 0xB8, 0x71, 0x14, 0x74, 0x41, 0xF4,
                                      0x69, 0xB6, 0xC4, 0x70, 0xA4, 0xB1, 0x5F, 0x7D, 0xFD, 0xE8, 0xCC, 0xE4, 0xC4, 0x62, 0x82, 0x5B};

    uint32_t sigtype = __builtin_bswap32(*(uint32_t *)cetk);
    if (sigtype != SIG_TYPE_RSA2048_SHA256) return 1;

    ticket_h *ticket = (ticket_h *)(cetk + sizeof(sigtype) + 0x13C);
    if (ticket->ticketCommonKeyYIndex != 1) return 1;

    if (!common_key_y_init) {
        uint8_t common_key_y[AES_BLOCK_SIZE] = {0};
        uint8_t *p9_base = (uint8_t *)0x08028000; // Process9 location on a regular boot (non-A9LH)
        uint32_t search_len = 0x70000;

        // A workaround is necessary when booting from A9LH
        if (A9LHBOOT) {
            uint8_t *firm_loc = (uint8_t *)fcram_temp + FCRAM_SPACING;
            firm_h *firm = (firm_h*)firm_loc;

            if (dump_firm(firm_loc, 0)) {
                print("Couldn't dump FIRM0!");
                return 1;
            }

            if (firm->magic != FIRM_MAGIC) {
                print("Couldn't decrypt FIRM0!");
                return 1;
            }

            // It can be assumed section 2 is ARM9 because it's either a 8.1 or a 9.0 N3DS FIRM
            if (decrypt_arm9bin((arm9bin_h *)(firm_loc + firm->section[2].offset), NATIVE_FIRM, 0)) {
                print("ARM9 binary couldn't be decrypted!");
                return 1;
            }

            p9_base = (uint8_t *)(firm_loc + firm->section[2].offset); // Beggining of the ARM9 section
            search_len = firm->section[2].size;
        }

        uint8_t tmp_hash[0x20];

        for (uint32_t i = 0; i < search_len; i ++) {
            if (*(p9_base + i) == 0x0C) { // First byte matches
                sha(tmp_hash, p9_base + i, 0x10, SHA_256_MODE);
                if (memcmp(tmp_hash, key_y_hash, 0x20) == 0) {
                    memcpy(common_key_y, p9_base + i, sizeof(common_key_y));
                    print("Found the common key Y");
                    break;
                }
            }
        }

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

int decrypt_firm_title(firm_h *dest, ncch_h *ncch, size_t *size, void *key)
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

    memcpy(dest, firm, *size);

    return 0;
}

int decrypt_firm(firm_h *dest, char *path_firmkey, char *path_cetk, size_t *size, enum firm_types firm_type)
{
    uint8_t firm_key[AES_BLOCK_SIZE];

    // Firmware is likely encrypted. Decrypt.
    if (read_file(firm_key, path_firmkey, AES_BLOCK_SIZE) != 0) {
        print("Failed to load FIRM key,\n  will try to create it...");

        if (read_file(fcram_temp, path_cetk, FCRAM_SPACING) != 0) {
            print("Failed to load CETK");

            if (firm_type == NATIVE_FIRM) {
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
    return 0;
}

int load_firm(firm_h *dest, char *path, char *path_firmkey, char *path_cetk, size_t *size, struct firm_signature *signatures, struct firm_signature **current, enum firm_types firm_type)
{
    struct firm_signature *firm_current = NULL;
    int status = 0;
    int firmware_changed = 0;

    if (read_file(dest, path, *size) != 0) {
        print("Failed to load FIRM");

        // Only whine about this if it's NATIVE_FIRM, which is important.
        if (firm_type == NATIVE_FIRM) {
            draw_loading("Failed to load FIRM", "Make sure the encrypted FIRM is\n  located at " PATH_FIRMWARE);
        }
        return 2;
    }
    print("Loaded FIRM");

    // Check and decrypt FIRM if it is encrypted.
    if (dest->magic != FIRM_MAGIC) {
        status = decrypt_firm(dest, path_firmkey, path_cetk, size, firm_type);
        if (status != 0)
            return status;
        firmware_changed = 1; // Decryption performed.
    } else {
        print("FIRM seems not encrypted");
    }

    // Determine firmware version
    firm_current = get_firm_info(dest, signatures);

    if (!firm_current) {
        print("Couldn't determine firmware version");
        draw_loading("Couldn't determine firmware version",
                     "The firmware you're trying to use is\n"
                     "  most probably not supported by Cakes.\n"
                     "Dumping it to your SD card:\n"
                     "  " PATH_UNSUPPORTED_FIRMWARE);
        write_file(dest, PATH_UNSUPPORTED_FIRMWARE, *size);
        print("Dumped unsupported firmware");
        return 1;
    }

    // The N3DS firm has an additional encryption layer for ARM9
    if (firm_current->console == console_n3ds) {
        // Look for the arm9 section
        for (firm_section_h *section = dest->section;
                section < dest->section + 4; section++) {
            if (section->type == FIRM_TYPE_ARM9) {
                // Check whether the arm9bin is encrypted.
                int arm9bin_iscrypt = 0;
                uint32_t magic = *(uint32_t*)((uintptr_t)dest + section->offset + 0x800);
                if (firm_type == NATIVE_FIRM)
                    arm9bin_iscrypt = (magic != ARM9BIN_MAGIC);
                else if (firm_type == AGB_FIRM || firm_type == TWL_FIRM)
                    arm9bin_iscrypt = (magic != LGY_ARM9BIN_MAGIC);

                if (arm9bin_iscrypt) {
                    // Decrypt the arm9bin.
                    if (decrypt_arm9bin((arm9bin_h *)((uintptr_t)dest + section->offset),
                                firm_type, firm_current->version) != 0) {
                        print("Couldn't decrypt ARM9 FIRM binary");
                        draw_loading("Couldn't decrypt ARM9 FIRM binary",
                                     "Double-check you've got the right firmware.bin.\n"
                                     "If you are trying to decrypt a >=9.6 firmware on a <9.6 console, please double-check your key is saved at:\n"
                                     "  " PATH_SLOT0X11KEY96 "\n"
                                     "We remind you that you can't decrypt it on an old 3ds.\nIf the issue persists, please file a bug report.");
                        return 1;
                    }
                    firmware_changed = 1; // Decryption of arm9bin performed.
                } else {
                    print("ARM9 FIRM binary seems not encrypted");
                    if (firm_type == NATIVE_FIRM && firm_current->version > 0x0F) {
                        slot0x11key96_init(); // This has to be loaded regardless, otherwise boot will fail.
                    }
                }

                // We assume there's only one section to decrypt.
                break;
            }
        }
    }

    // Save firmware.bin if decryption was done.
    if (firmware_changed) {
        print("Saving decrypted FIRM");
        write_file(dest, path, *size);
    }

    if (firm_current->console == console_n3ds) {
        print("Fixing arm9 entrypoint...");

        // Patch the entrypoint to skip arm9loader
        if (firm_type == NATIVE_FIRM) {
            dest->a9Entry = 0x0801B01C;
        } else if (firm_type == TWL_FIRM ||
                firm_type == AGB_FIRM) {
            dest->a9Entry = 0x0801301C;
        }
        // The entrypoints seem to be the same across different FIRM versions,
        //  so we don't change them.
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
    if (update_96_keys && current_firm->console == console_n3ds && current_firm->version > 0x0F) {
        void *keydata = NULL;
        if (current_firm->version == 0x1B || current_firm->version == 0x1F) {
            keydata = (void *)((uintptr_t)firm_loc + firm_loc->section[2].offset + 0x89814);
        } else if (current_firm->version == 0x21) {
            keydata = (void *)((uintptr_t)firm_loc + firm_loc->section[2].offset + 0x89A14);
        }

        aes_use_keyslot(0x11);
        uint8_t keyx[AES_BLOCK_SIZE];
        for (int slot = 0x19; slot < 0x20; slot++) {
            aes(keyx, keydata, 1, NULL, AES_ECB_DECRYPT_MODE, 0);
            aes_setkey(slot, keyx, AES_KEYX, AES_INPUT_BE | AES_INPUT_NORMAL);
            *(uint8_t *)(keydata + 0xF) += 1;
        }

        print("Updated keyX keyslots");
    }

    struct memory_header *memory = (void *)(memory_loc + 1);
    print("Started copying");
    while ((uintptr_t)memory < (uintptr_t)memory_loc + *memory_loc) {
        memcpy((void *)memory->location, memory + 1, memory->size);
        memory = (void *)((uintptr_t)(memory + 1) + memory->size);
    }
    print("Copied memory");

    for (firm_section_h *section = firm_loc->section;
            section < firm_loc->section + 4 && section->address != 0; section++) {
        memcpy((void *)section->address, (void *)firm_loc + section->offset, section->size);
    }
    print("Copied FIRM");

    *a11_entry = (uint32_t)disable_lcds;
    while (*a11_entry);  // Make sure it jumped there correctly before changing it.
    *a11_entry = (uint32_t)firm_loc->a11Entry;
    print("Prepared arm11 entry");

    print("Booting...");

    ((void (*)())firm_loc->a9Entry)();
}

int load_firms()
{
    const char *title = "Loading firm";

    print("Loading NATIVE_FIRM...");
    draw_loading(title, "Loading NATIVE_FIRM...");
    if (load_firm(firm_orig_loc, PATH_FIRMWARE, PATH_FIRMKEY, PATH_CETK, &firm_size, firm_signatures, &current_firm, NATIVE_FIRM) != 0) {
        draw_string(screen_top_left, "FIRM that failed: NATIVE_FIRM",
                0, SCREEN_TOP_HEIGHT - MARGIN_VERT - SPACING_VERT, COLOR_NEUTRAL);
        return 1;
    }

    print("Loading TWL_FIRM...");
    draw_loading(title, "Loading TWL_FIRM...");
    if (load_firm(twl_firm_orig_loc, PATH_TWL_FIRMWARE, PATH_TWL_FIRMKEY, PATH_TWL_CETK, &twl_firm_size, twl_firm_signatures, &current_twl_firm, TWL_FIRM) == 1) {
        draw_string(screen_top_left, "FIRM that failed: TWL_FIRM",
                0, SCREEN_TOP_HEIGHT - MARGIN_VERT - SPACING_VERT, COLOR_NEUTRAL);
        return 1;
    }

    print("Loading AGB_FIRM...");
    draw_loading(title, "Loading AGB_FIRM...");
    if (load_firm(agb_firm_orig_loc, PATH_AGB_FIRMWARE, PATH_AGB_FIRMKEY, PATH_AGB_CETK, &agb_firm_size, agb_firm_signatures, &current_agb_firm, AGB_FIRM) == 1) {
        draw_string(screen_top_left, "FIRM that failed: AGB_FIRM",
                0, SCREEN_TOP_HEIGHT - MARGIN_VERT - SPACING_VERT, COLOR_NEUTRAL);
        return 1;
    }

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

    if ((save_firm || config->autoboot_enabled) &&
            (patches_modified || f_stat(PATH_MEMORY, NULL) != 0)) {
        draw_loading(title, "Saving Memory...");
        print("Saving memory");
        if (write_file(memory_loc, PATH_MEMORY, *memory_loc) != 0) {
            draw_message("Failed to save the patched FIRM", "For some reason, we haven't been able to write to the SD card.");
            return;
        }
    }

    if (current_twl_firm && (patches_modified || f_stat(PATH_PATCHED_TWL_FIRMWARE, NULL) != 0)) {
        draw_loading(title, "Saving TWL_FIRM...");
        print("Saving patched TWL_FIRM");
        if (write_file(twl_firm_loc, PATH_PATCHED_TWL_FIRMWARE, twl_firm_size) != 0) {
            draw_message("Failed to save the patched FIRM", "For some reason, we haven't been able to write to the SD card.");
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
#endif
