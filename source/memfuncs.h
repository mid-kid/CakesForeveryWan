#ifndef __memfuncs_h__
#define __memfuncs_h__

#include <stdint.h>

int strlen(const char *string);
void memcpy32(void *dest, const void *src, uint32_t size);
void memset32(void *dest, uint32_t filler, uint32_t size);
void memset(void *dest, int filler, uint32_t size);
int memcmp(const void *buf1, const void *buf2, uint32_t size);
void memcpy(void *dest, const void *src, uint32_t size);
void strncpy(void *dest, const void *src, uint32_t size);
int strncmp(const void *buf1, const void *buf2, uint32_t size);

#endif
