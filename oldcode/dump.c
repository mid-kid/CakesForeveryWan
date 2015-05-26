#include "dump.h"

#include <stdint.h>
#include "draw.h"

static int (*fopen)(uint32_t (*handle)[], short unsigned int *path, uint32_t flags) = (void *)0x001B82A8;
static uint32_t (*fread)(uint32_t (*handle)[], uint32_t *read, void *buffer, uint32_t size) = (void *)0x001B3954;

int dump_mset_memory()
{
    uint32_t file_handle[8] = {0};
    uint32_t bytes_written = 0;

    print("Dumping MSET memory");
    int result = fopen(&file_handle, L"YS:/mset_memory.bin", 6);
    print("Done fopen");
    if (result != 0) {
        print("Fopen failed!");
        return 1;
    }

    fwrite(&file_handle, &bytes_written, (void *)0x100000, 0x300000);
    print("Done fwrite");
    if (bytes_written != 0x300000) {
        print("Fwrite failed!");
        return 1;
    }

    print("Dumped MSET memory!");

    return 0;
}
