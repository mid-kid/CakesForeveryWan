#include "misc.h"

int strlen(char *string)
{
    char *string_end = string;
    while (*string_end) string_end++;
    return string_end - string;
}
