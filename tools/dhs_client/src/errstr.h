#ifndef DHS_CLIENT_ERRSTR_H
#define DHS_CLIENT_ERRSTR_H

#include <stdint.h>
#include <stddef.h>

void errstr(uint32_t errCode, const char** descStr, const char** summaryStr, const char** moduleStr, const char** levelStr);

#endif /*DHS_CLIENT_ERRSTR_H*/
