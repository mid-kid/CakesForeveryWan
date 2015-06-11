#include "firm.h"

#include "fatfs/ff.h"
#include "../draw.h"
#include "hid.h"

static void *firm_loc = (void *)0x24000000;

// TODO: Put this in a nice place
void memcpy32(void *dest, void *src, uint32_t size)
{
    uint32_t *dest32 = (uint32_t *)dest;
    uint32_t *src32 = (uint32_t *)src;
    for (uint32_t i = 0; i < size / 4; i++) {
        dest32[i] = src32[i];
    }
}

void read_firm()
{
    FIL handle;
    unsigned int bytes_read = 0;

    // TODO: Check errors
    print("Opening file");
    f_open(&handle, "/firm.bin", 1);
    print("Reading file");
    f_read(&handle, firm_loc, 0xEB000, &bytes_read);
    print("Closing file");
    f_close(&handle);
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
