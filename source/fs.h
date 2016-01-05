#pragma once

int mount_sd();
int unmount_sd();
int read_file_offset(void *dest, const char *path, unsigned int size, unsigned int offset);
int write_file(const void *buffer, const char *path, unsigned int size);

#define read_file(dest, path, size) read_file_offset(dest, path, size, 0)
