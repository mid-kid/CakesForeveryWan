#include "fs.h"

#include <stddef.h>
#include "draw.h"
#include "fatfs/ff.h"
#include "memfuncs.h"

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

int find_file_pattern(char buffer[][_MAX_LFN + 1], const char *dirpath, int pathlen, int max_entries, const char *pattern)
{
    int count = 0;

    FRESULT fr;
    DIR dj;
    FILINFO fno;

    fr = f_findfirst(&dj, &fno, dirpath, pattern);

    while (fr == FR_OK && fno.fname[0] && count < max_entries) {
        memcpy(buffer[count], dirpath, pathlen);
        buffer[count][pathlen] = '/';
        strncpy(&buffer[count][pathlen + 1], fno.fname, _MAX_LFN + 1 - pathlen + 1);
        fr = f_findnext(&dj, &fno);
        count++;
    }

    f_closedir(&dj);

    return count;
}