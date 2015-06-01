#include <stdint.h>
#include "firmcompat.h"
#include "appcompat.h"
#include "memchunkhax.h"
#include "firmlaunchax.h"
#include "draw.h"

int load_file(char *dest, short unsigned int *path, uint32_t offset, uint32_t size)
{
    uint32_t file_handle[8] = {0};
    uint32_t bytes_read = 0;

    int result = fopen(&file_handle, path, 1);
    if (result != 0) {
        print("Fopen failed!");
        return 1;
    }
    file_handle[1] = offset;

    fread(&file_handle, &bytes_read, dest, size);
    print("File has been loaded");

    return 0;
}

__attribute__((naked))
void arm11_kernel_code()
{
    __asm__ volatile ("clrex");
    print("I'm in!");

    // Reboot and load our arm9 payload in arm9 kernel mode
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

    // Load the arm9 payload to memory
    // Spider has size restrictions to the Launcher, so we need to load the arm9
    //   payload separately.
    result = load_file((char *)(0x14000000 + APP_CFW_OFFSET),
                       APP_LAUNCHER_PATH, 0x20000, 0x10000);
    if (result != 0) return;
    print("Loaded firmware!");

    // Now, we gain arm11 kernel mode
    print("Doing memchunkhax to gain arm11");
    memchunk_arm11hax(arm11_kernel_code);
}

void main()
{
    clear_screens();
    print("Hello world!");

    do_firmlaunch();
}
