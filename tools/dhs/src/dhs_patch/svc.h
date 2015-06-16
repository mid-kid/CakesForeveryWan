#ifndef __3DSSVC_H
#define __3DSSVC_H

#include <stdint.h>
typedef uint32_t Handle;

void svcExitThread() __attribute__((naked));
void svcSleepThread(uint32_t nanoSecsLo, uint32_t nanoSecsHi) __attribute__((naked));
void svcFlushProcessDataCache(Handle process, void const* addr, uint32_t size) __attribute__((naked));
void svcBackdoor(void* funcptr) __attribute__((naked));
void svcCreateThread(Handle* thread, void(*entrypoint)(void), void* arg, void* stacktop, int32_t threadpriority, int32_t processorid);

#endif /*__3DSSVC_H*/
