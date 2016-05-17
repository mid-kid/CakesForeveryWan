#pragma once

#include "fatfs/ff.h"

int mount_sd();
int unmount_sd();
int read_file_offset(void *dest, const char *path, unsigned int size, unsigned int offset);
int write_file(const void *buffer, const char *path, unsigned int size);
int find_file_pattern(char buffer[][_MAX_LFN + 1], const char *dirpath, int pathlen, int max_entries, const char *pattern);

#define read_file(dest, path, size) read_file_offset(dest, path, size, 0)
