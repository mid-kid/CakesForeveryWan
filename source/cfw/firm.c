#include "firm.h"

#include <stdint.h>
#include <draw.h>
#include <memfuncs.h>
#include "fs.h"
#include "menu.h"
#include "crypto.h"
#include "patch.h"
#include "fatfs/ff.h"

void *firm_loc = (void *)0x23000000;
static void *firm_loc_encrypted = (void *)0x24100000;
static const int firm_size_encrypted = 0xF0000;

static uint8_t firm_key[16] = {0};

int save_firm = 0;
uint8_t firm_ver = 0;
const char *save_path = "/cakes/patched_firm.bin";

// We use the firm's section 0's hash to identify the version
firm_sig_h firm_sig[] = {
    {.sig = {0xEE, 0xE2, 0x81, 0x2E, 0xB9, 0x10, 0x0D, 0x03, 0xFE, 0xA2, 0x3F, 0x44, 0xB5, 0x1C, 0xB3, 0x5E},
     .ver = 0x1F},
    {.sig = {0x3F, 0xBF, 0x14, 0x06, 0x33, 0x77, 0x82, 0xDE, 0xB2, 0x68, 0x83, 0x01, 0x6B, 0x1A, 0x71, 0x69},
     .ver = 0x38},
    {.sig = {0x5C, 0x6A, 0x51, 0xF3, 0x79, 0x4D, 0x21, 0x91, 0x0B, 0xBB, 0xFD, 0x17, 0x7B, 0x72, 0x6B, 0x59},
     .ver = 0x49},
    {.ver = 0}
};

int prepare_files()
{
    int rc;

    rc = read_file(firm_loc_encrypted, "/firmware.bin", 0);
    if (rc != 0) {
        print("Failed to load FIRM");
        draw_message("Failed to load FIRM", "Make sure the encrypted FIRM is\n  located at /firmware.bin");
        return 1;
    }
    print("Loaded FIRM");

    rc = read_file(firm_key, "/cakes/firmkey.bin", 16);
    if (rc != 0) {
        print("Failed to load FIRM key");
        draw_message("Failed to load FIRM key", "Make sure the FIRM key is\n  located at /cakes/firmkey.bin");
        return 1;
    }
    print("Loaded FIRM key");

    return 0;
}

int decrypt_firm()
{
    void *curloc = firm_loc_encrypted;
    uint32_t cursize = firm_size_encrypted;
    uint8_t firm_iv[16] = {0};
    uint8_t exefs_key[16] = {0};
    uint8_t exefs_iv[16] = {0};

    print("Decrypting the NCCH");
    aes_setkey(0x11, firm_key, AES_KEYNORMAL, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_use_keyslot(0x11);
    aes(curloc, curloc, cursize / AES_BLOCK_SIZE, firm_iv, AES_CBC_DECRYPT_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);

    if (*(uint32_t *)(curloc + 0x100) != NCCH_MAGIC) {
        print("Failed to decrypt the NCCH");
        draw_message("Failed to decrypt the NCCH", "Please double check your firmware.bin and firmkey.bin are right.");
        return 1;
    }

    ncch_h *ncch = (ncch_h *)curloc;

    memcpy(exefs_key, ncch, 16);
    ncch_getctr(ncch, exefs_iv, NCCHTYPE_EXEFS);

    // Get the exefs offset and size from the NCCH
    cursize = ncch->exeFSSize * MEDIA_UNITS;
    curloc += ncch->exeFSOffset * MEDIA_UNITS;

    print("Decrypting the exefs");
    aes_setkey(0x2C, exefs_key, AES_KEYY, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_use_keyslot(0x2C);
    aes(curloc, curloc, cursize / AES_BLOCK_SIZE, exefs_iv, AES_CTR_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);

    // Get the decrypted FIRM
    // We assume the firm.bin is always the first file
    exefs_h *exefs = (exefs_h *)curloc;
    cursize = exefs->fileHeaders[0].size;
    curloc += sizeof(exefs_h);  // The offset right behind the exefs header; the first file.

    print("Copying the FIRM");
    memcpy32(firm_loc, curloc, cursize);

    firm_h *firm = (firm_h *)firm_loc;
    if (firm->magic != FIRM_MAGIC) {
        print("Failed to decrypt the exefs");
        draw_message("Failed to decrypt the exefs", "I just don't know what went wrong");
        return 1;
    }

    // Determine firmware version
    for (int i = 0; firm_sig[i].ver != 0; i++) {
        if (memcmp(firm_sig[i].sig, firm->section[0].hash, 0x10) == 0) {
            firm_ver = firm_sig[i].ver;
        }
    }

    return 0;
}

uint8_t get_firm_ver()
{
    return firm_ver;
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

    firm_h *firm = (firm_h *)firm_loc;
    firm_section_h *sections = firm->section;

    memcpy32(sections[0].address, firm_loc + sections[0].offset, sections[0].size);
    memcpy32(sections[1].address, firm_loc + sections[1].offset, sections[1].size);
    memcpy32(sections[2].address, firm_loc + sections[2].offset, sections[2].size);
    print("Copied FIRM");

    *(uint32_t *)0x1FFFFFF8 = (uint32_t)firm->a11Entry;
    print("Prepared arm11 entry");

    print("Booting...");
    ((void (*)())*(void **)(firm_loc + 0xC))();
}

int load_firm()
{
    char *title = "Loading firm";

    draw_loading(title, "Loading...");
    if (prepare_files() != 0) return 1;

    draw_loading(title, "Decrypting...");
    if (decrypt_firm() != 0) return 1;

    return 0;
}

void boot_cfw()
{
    char *title = "Booting CFW";

    draw_loading(title, "Patching...");
    if (patch_firm_all() != 0) return;

    if (save_firm) {
        draw_loading(title, "Saving FIRM...");
        print("Saving patched FIRM");
        if (write_file(firm_loc, save_path, firm_size) != 0) {
            draw_message("Failed to save FIRM", "One or more patches you selected requires this.\nBut, for some reason, I failed to write it.");
            return;
        }
    }

    draw_loading(title, "Booting...");
    boot_firm();
}
