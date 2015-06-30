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

#ifdef ARM9
void memcpy(void *dest, const void *src, uint32_t size)
{
    char *destc = (char *)dest;
    char *srcc = (char *)src;
    for (uint32_t i = 0; i < size; i++) {
        destc[i] = srcc[i];
    }
}
#endif

void memset(void *dest, int filler, uint32_t size)
{
    char *destc = (char *)dest;
    for (uint32_t i = 0; i < size; i++) {
        destc[i] = filler;
    }
}

int memcmp(void *buf1, void *buf2, uint32_t size)
{
    char *buf1c = (char *)buf1;
    char *buf2c = (char *)buf2;
    for (uint32_t i = 0; i < size; i++) {
        int cmp = buf1c[i] - buf2c[i];
        if (cmp) {
            return cmp;
        }
    }

    return 0;
}
