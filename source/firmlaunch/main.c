#include "main.h"

#include <stdint.h>
#include "firmcompat.h"
#include "memchunkhax.h"
#include "firmlaunchax.h"
#include "../mset/draw.h"

// MSET functions
int (*fopen)(uint32_t (*handle)[], short unsigned int *path, uint32_t flags) = (void *)0x001B82A8;
uint32_t (*fread)(uint32_t (*handle)[], uint32_t *read, void *buffer, uint32_t size) = (void *)0x001B3954;


int load_file(char *dest, short unsigned int *path, uint32_t size)
{
    uint32_t file_handle[8] = {0};
    uint32_t bytes_read = 0;

    int result = fopen(&file_handle, path, 1);
    if (result != 0) {
        print("Fopen failed!");
        return 1;
    }

    fread(&file_handle, &bytes_read, dest, size);
    if (bytes_read != size) {
        print("Fread failed!");
        return 1;
    }

    print("File has been loaded");

    return 0;
}

__attribute__((naked))
void arm11_kernel_code()
{
    __asm__ volatile ("clrex");
    print("I'm in!");

    // Reboot and load our arm9 payload
    firmlaunch_arm9hax();

    // We should never return here
    while (1) {};
}

void do_firmlaunch()
{
    int result;

    // Some offsets differ per firmware
    result = set_firmware_offsets();
    if (result != 0) return;
    print("Got firmware-specific offsets");

    // This is used later in arm9 to boot into it.
    result = load_file((char *)0x14400000, L"YS:/firm.bin", 0xEB000);
    if (result != 0) return;
    print("Loaded firmware!");

    // Now, we gain arm11 kernel mode
    print("Doing memchunkhax to gain arm11");
    memchunk_arm11hax(arm11_kernel_code);
}
