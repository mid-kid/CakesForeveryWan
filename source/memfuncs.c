#include "memfuncs.h"

#include <stdint.h>
#include <stddef.h>

int strlen(const char *string)
{
    char *string_end = (char *)string;
    while (*string_end) string_end++;
    return string_end - string;
}

void memcpy(void *dest, const void *src, size_t size)
{
    char *destc = (char *)dest;
    const char *srcc = (const char *)src;

    // If we can align both dest and src together...
    if ((uintptr_t)srcc % sizeof(size_t) == (uintptr_t)destc % sizeof(size_t)) {
        // Align them and copy faster
        while ((uintptr_t)srcc % sizeof(size_t) && size--) {
            *destc++ = *srcc++;
        }

        for (; size >= sizeof(size_t); size -= sizeof(size_t),
                destc += sizeof(size_t), srcc += sizeof(size_t)) {
            *(size_t *)destc = *(size_t *)srcc;
        }
    }

    // Finish by copying the leftovers
    while (size--) {
        *destc++ = *srcc++;
    }
}

void memmove(void *dest, const void *src, size_t size)
{
    // memcpy does the job of moving backwards just fine
    if (dest < src || src + size <= dest) {
        return memcpy(dest, src, size);
    }

    // Moving forward is just a reverse memcpy
    char *destc = (char *)dest;
    const char *srcc = (const char *)src;

    // If we can align both dest and src together...
    if ((uintptr_t)srcc % sizeof(size_t) == (uintptr_t)destc % sizeof(size_t)) {
        // Align them and copy faster
        while ((uintptr_t)(destc + size) % sizeof(size_t) && size--) {
            destc[size] = srcc[size];
        }

        while (size >= sizeof(size_t)) {
            size -= sizeof(size_t);
            *(size_t *)(destc + size) = *(size_t *)(srcc + size);
        }
    }

    // Finish by copying the leftovers
    while (size--) {
        destc[size] = srcc[size];
    }
}

void memset(void *dest, const int filler, size_t size)
{
    char *destc = (char *)dest;

    // Align dest to 4 bytes
    while ((uintptr_t)destc % sizeof(size_t) && size--) {
        *destc++ = filler;
    }

    // Set 32 bytes at a time
    for (; size >= sizeof(size_t); size -= sizeof(size_t), destc += sizeof(size_t)) {
        *(size_t *)destc = filler;
    }

    // Finish
    while (size--) {
        *destc++ = filler;
    }
}

int memcmp(const void *buf1, const void *buf2, const size_t size)
{
    const char *buf1c = (const char *)buf1;
    const char *buf2c = (const char *)buf2;
    for (size_t i = 0; i < size; i++) {
        int cmp = buf1c[i] - buf2c[i];
        if (cmp) {
            return cmp;
        }
    }

    return 0;
}

void strncpy(void *dest, const void *src, const size_t size)
{
    char *destc = (char *)dest;
    const char *srcc = (const char *)src;

    size_t i;
    for (i = 0; i < size && srcc[i] != 0; i++) {
        destc[i] = srcc[i];
    }

    // Make sure the resulting string is terminated.
    destc[i] = 0;
}

int strncmp(const void *buf1, const void *buf2, const size_t size)
{
    const char *buf1c = (const char *)buf1;
    const char *buf2c = (const char *)buf2;

    size_t i;
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
