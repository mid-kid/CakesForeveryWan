#include "fs.h"

#include <draw.h>
#include "fatfs/ff.h"

static FATFS fs;

void mount_sd()
{
    if (f_mount(&fs, "0:", 1) == FR_OK) {
        print("Mounted SD card");
    } else {
        print("Failed to mount SD card!");
    }
}

void unmount_sd()
{
    f_mount((void *)0, "0:", 1);
    print("Unmounted SD card");
}

int read_file_offset(void *dest, const char *path, unsigned int size, unsigned int offset)
{
    FRESULT fr;
    FIL handle;
    unsigned int bytes_read = 0;

    fr = f_open(&handle, path, FA_READ);
    if (fr != FR_OK) return fr;

    if (offset) {
        fr = f_lseek(&handle, offset);
        if (fr != FR_OK) return fr;
    }

    fr = f_read(&handle, dest, size, &bytes_read);
    if (fr != FR_OK) return fr;

    fr = f_close(&handle);
    if (fr != FR_OK) return fr;

    return 0;
}

int write_file(const void *buffer, const char *path, unsigned int size)
{
    FRESULT fr;
    FIL handle;
    unsigned int bytes_written = 0;

    fr = f_open(&handle, path, FA_WRITE | FA_OPEN_ALWAYS);
    if (fr != FR_OK) return fr;

    fr = f_write(&handle, buffer, size, &bytes_written);
    if (fr != FR_OK || bytes_written != size) return fr;

    // This for some reason always returns an error
    f_close(&handle);
    if (fr != FR_OK) return fr;

    return 0;
}
