#ifndef __ERRSTR_H_
#define __ERRSTR_H_

#include <stdint.h>
#include <stddef.h>

void errstr(uint32_t errCode, const char** descStr, const char** summaryStr, const char** moduleStr, const char** levelStr);

#endif /*__ERRSTR_H_*/
