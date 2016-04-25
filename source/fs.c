#include "fs.h"

#include <stddef.h>
#include "draw.h"
#include "fatfs/ff.h"

static FATFS fs;

int mount_sd()
{
    if (f_mount(&fs, "0:", 1) != FR_OK) {
        print("Failed to mount SD card!");
        return 1;
    }
    return 0;
}

int unmount_sd()
{
    if (f_mount(NULL, "0:", 1) != FR_OK) {
        print("Failed to mount SD card!");
        return 1;
    }
    print("Unmounted SD card");
    return 0;
}

int read_file_offset(void *dest, const char *path, unsigned int size, unsigned int offset)
{
    FRESULT fr;
    FIL handle;
    unsigned int bytes_read = 0;

    fr = f_open(&handle, path, FA_READ);
    if (fr != FR_OK) goto error;

    if (!size) {
        size = f_size(&handle);
    }

    if (offset) {
        fr = f_lseek(&handle, offset);
        if (fr != FR_OK) goto error;
    }

    fr = f_read(&handle, dest, size, &bytes_read);
    if (fr != FR_OK) goto error;

    fr = f_close(&handle);
    if (fr != FR_OK) goto error;

    return 0;

error:
    f_close(&handle);
    return fr;
}

int write_file(const void *buffer, const char *path, unsigned int size)
{
    FRESULT fr;
    FIL handle;
    unsigned int bytes_written = 0;

    fr = f_open(&handle, path, FA_WRITE | FA_OPEN_ALWAYS);
    if (fr != FR_OK) goto error;

    fr = f_write(&handle, buffer, size, &bytes_written);
    if (fr != FR_OK || bytes_written != size) goto error;

    // For some reason this always returns 1
    f_close(&handle);

    return 0;

error:
    f_close(&handle);
    return fr;
}
