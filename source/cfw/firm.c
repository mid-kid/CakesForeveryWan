#include "firm.h"

#include <stdint.h>
#include <draw.h>
#include <memfuncs.h>
#include "fs.h"
#include "menu.h"
#include "crypto.h"
#include "patch.h"
#include "fatfs/ff.h"

void *firm_loc = (void *)0x24000000;
const int firm_size = 0xEB000;
static void *firm_loc_encrypted = (void *)0x24100000;
static const int firm_size_encrypted = 0xEBC00;

static uint8_t firm_key[16] = {0};

static const uint32_t ncch_magic = 0x4843434E;  // Hex for "NCCH".
static const uint32_t firm_magic = 0x4D524946;  // Hex for "FIRM". For easy comparing.

int save_firm = 0;
const char *save_path = "/cakes/patched_firm.bin";

int prepare_files()
{
    int rc;

    rc = read_file(firm_loc_encrypted, "/firmware.bin", firm_size_encrypted);
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

    if (*(uint32_t *)(curloc + 0x100) != ncch_magic) {
        print("Failed to decrypt the NCCH");
        draw_message("Failed to decrypt the NCCH", "Please double check your firmware.bin and firmkey.bin are right.");
        return 1;
    }

    memcpy(exefs_key, curloc, 16);
    ncch_getctr(curloc, exefs_iv, NCCHTYPE_EXEFS);

    // Get the exefs offset and size from the NCCH
    cursize = *(uint32_t *)(curloc + 0x1A4) * 0x200;
    curloc += *(uint32_t *)(curloc + 0x1A0) * 0x200;

    print("Decrypting the exefs");
    aes_setkey(0x2C, exefs_key, AES_KEYY, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_use_keyslot(0x2C);
    aes(curloc, curloc, cursize / AES_BLOCK_SIZE, exefs_iv, AES_CTR_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);

    // Get the decrypted FIRM
    // We assume the firm.bin is always the first file
    cursize = firm_size;
    curloc += 0x200;  // The offset right behind the exefs header; the first file.

    print("Copying the FIRM");
    memcpy32(firm_loc, curloc, cursize);

    if (*(uint32_t *)firm_loc != firm_magic) {
        print("Failed to decrypt the exefs");
        draw_message("Failed to decrypt the exefs", "I just don't know what went wrong");
        return 1;
    }

    return 0;
}

void boot_firm()
{
    print("Booting FIRM...");

    __asm__ (
        "push {r4-r12}\n\t"
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
        "pop {r4-r12}\n\t"
    );
    print("Set up MPU");

    struct firm_section *sections = firm_loc + 0x40;
    memcpy32(sections[0].address, firm_loc + (uintptr_t)sections[0].offset, sections[0].size);
    memcpy32(sections[1].address, firm_loc + (uintptr_t)sections[1].offset, sections[1].size);
    memcpy32(sections[2].address, firm_loc + (uintptr_t)sections[2].offset, sections[2].size);
    print("Copied FIRM");

    *(uint32_t *)0x1FFFFFF8 = *(uint32_t *)(firm_loc + 8);
    print("Prepared arm11 entry");

    print("Booting...");
    ((void (*)())*(void **)(firm_loc + 0xC))();
}

void boot_cfw(int patch_level)
{
    char *title = "Booting CFW";

    draw_loading(title, "Loading...");
    if (prepare_files() != 0) return;

    draw_loading(title, "Decrypting...");
    if (decrypt_firm() != 0) return;

    draw_loading(title, "Patching...");
    if (patch_firm_all(patch_level) != 0) return;

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
