#pragma once

#include <stdint.h>

int mount_sd();
int unmount_sd();
int read_file_offset(void *dest, const char *path, uint32_t size, uint32_t offset);
int write_file(const void *buffer, const char *path, uint32_t size);

#define read_file(dest, path, size) read_file_offset(dest, path, size, 0)
