#include "memfuncs.h"

#include <stdint.h>

int strlen(const char *string)
{
    char *string_end = (char *)string;
    while (*string_end) string_end++;
    return string_end - string;
}

void memcpy32(void *dest, const void *src, uint32_t size)
{
    uint32_t *dest32 = (uint32_t *)dest;
    uint32_t *src32 = (uint32_t *)src;
    for (uint32_t i = 0; i < size / 4; i++) {
        dest32[i] = src32[i];
    }
}

void memset32(void *dest, uint32_t filler, uint32_t size)
{
    uint32_t *dest32 = (uint32_t *)dest;
    for (uint32_t i = 0; i < size / 4; i++) {
        dest32[i] = filler;
    }
}

void memcpy(void *dest, const void *src, uint32_t size)
{
    char *destc = (char *)dest;
    const char *srcc = (const char *)src;
    for (uint32_t i = 0; i < size; i++) {
        destc[i] = srcc[i];
    }
}

void memset(void *dest, int filler, uint32_t size)
{
    char *destc = (char *)dest;
    for (uint32_t i = 0; i < size; i++) {
        destc[i] = filler;
    }
}

int memcmp(const void *buf1, const void *buf2, uint32_t size)
{
    const char *buf1c = (const char *)buf1;
    const char *buf2c = (const char *)buf2;
    for (uint32_t i = 0; i < size; i++) {
        int cmp = buf1c[i] - buf2c[i];
        if (cmp) {
            return cmp;
        }
    }

    return 0;
}

void strncpy(void *dest, const void *src, uint32_t size)
{
    char *destc = (char *)dest;
    const char *srcc = (const char *)src;

    uint32_t i;
    for (i = 0; i < size && srcc[i] != 0; i++) {
        destc[i] = srcc[i];
    }

    // Make sure the resulting string is terminated.
    destc[i] = 0;
}

int strncmp(const void *buf1, const void *buf2, uint32_t size)
{
    const char *buf1c = (const char *)buf1;
    const char *buf2c = (const char *)buf2;

    uint32_t i;
    for (i = 0; i < size && buf1c[i] != 0 && buf2c[i] != 0; i++) {
        int cmp = buf1c[i] - buf2c[i];
        if (cmp) {
            return cmp;
        }
    }

    // Make sure the strings end at the same offset, if they end.
    if ((buf1c[i] == 0 || buf2c[i] == 0) && (buf1c[i] != 0 || buf2c[i] != 0)) {
        return -1;
    }

    return 0;
}

int atoi(const char *str)
{
    int res = 0;
    while (*str && *str >= '0' && *str <= '9') {
        res =  *str - '0' + res * 10;
        str++;
    }

    return res;
}
