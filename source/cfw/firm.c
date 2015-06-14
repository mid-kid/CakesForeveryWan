#include "firm.h"

#include <stdint.h>
#include <draw.h>
#include <memfuncs.h>
#include "fatfs/ff.h"
#include "hid.h"
#include "menu.h"
#include "crypto.h"

static void *firm_loc_encrypted = (void *)0x24100000;
static void *firm_loc = (void *)0x24000000;
static const int firm_size_encrypted = 0xEBC00;
static const int firm_size = 0xEB000;

static const int offset_exefs = 0xA00;
static const int offset_firm = 0x200;

uint8_t firm_key[16] = {0};
uint8_t firm_iv[16] = {0};
uint8_t exefs_key[16] = {0};
uint8_t exefs_iv[16] = {0};

int read_file(void *dest, char *path, unsigned int size)
{
    FRESULT fr;
    FIL handle;
    unsigned int bytes_read = 0;

    fr = f_open(&handle, path, 1);
    if (fr != FR_OK) return 1;

    fr = f_read(&handle, dest, size, &bytes_read);
    if (fr != FR_OK || bytes_read != size) return 1;

    fr = f_close(&handle);
    if (fr != FR_OK) return 1;

    return 0;
}

int prepare_files()
{
    int rc;

    rc = read_file(firm_loc_encrypted, "/firm.bin", firm_size_encrypted);
    if (rc != 0) {
        print("Failed to load FIRM");
        draw_message("Failed to load FIRM", "Make sure the encrypted FIRM is\n  located at /firm.bin");
        return 1;
    }
    print("Loaded FIRM");

    rc = read_file(firm_key, "/firmkey.bin", 16);
    if (rc != 0) {
        print("Failed to load FIRM key");
        draw_message("Failed to load FIRM key", "Make sure the FIRM key is\n  located at /firmkey.bin");
    }
    print("Loaded FIRM key");

    return 0;
}

void decrypt_firm()
{
    void *curloc = firm_loc_encrypted;
    int cursize = firm_size_encrypted;

    print("Decrypting the NCCH");
    aes_setkey(0x11, firm_key, AES_KEYNORMAL, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_use_keyslot(0x11);
    aes(curloc, curloc, cursize / AES_BLOCK_SIZE, firm_iv, AES_CBC_DECRYPT_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);

    print("Getting exefs key");
    memcpy(exefs_key, curloc, 16);
    ncch_getctr(curloc, exefs_iv, NCCHTYPE_EXEFS);

    // Get the exefs
    curloc += offset_exefs;
    cursize -= offset_exefs;

    print("Decrypting the exefs");
    aes_setkey(0x2C, exefs_key, AES_KEYY, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_use_keyslot(0x2C);
    aes(curloc, curloc, cursize / AES_BLOCK_SIZE, exefs_iv, AES_CTR_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);

    // Get the decrypted FIRM
    curloc += offset_firm;
    cursize -= offset_firm;

    print("Copying the FIRM");
    memcpy32(firm_loc, curloc, cursize);

    print("Result:");
    print(firm_loc);
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

    // TODO: Clean this up
    uint32_t *thing = firm_loc + 0x40;
    memcpy32((void *)thing[1], *thing + firm_loc, thing[2]);
    uint32_t *thing2 = firm_loc + 0x70;
    memcpy32((void *)thing2[1], *thing2 + firm_loc, thing2[2]);
    uint32_t *thing3 = firm_loc + 0xA0;
    memcpy32((void *)thing3[1], *thing3 + firm_loc, thing3[2]);
    print("Copied FIRM");

    *(uint32_t *)0x1FFFFFF8 = *(uint32_t *)(firm_loc + 8);
    print("Prepared arm11 entry");

    print("Press any button to boot.");
    wait_key();
    ((void (*)())*(void **)(firm_loc + 0xC))();
}

void boot_cfw()
{
    draw_loading("Booting CFW", "Loading...");   
    if (prepare_files() != 0) return;

    draw_loading("Booting CFW", "Decrypting...");
    decrypt_firm();

    draw_loading("Booting CFW", "Booting...");
    boot_firm();
}
