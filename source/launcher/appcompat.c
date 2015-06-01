#include "appcompat.h"

// TODO: This is shit.
void (*memcpy)(void *dest, void *src, int len) = (void *)FUNC_MEMCPY;
int (*GSPGPU_FlushDataCache)(void *address, uint32_t length) = (void *)FUNC_GSPGPU_FLUSHDATACACHE;
void (*nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue)(void *arg1, void *arg2) = (void *)FUNC_NN__GXLOW__CTR__CMDREQQUEUETX__TRYENQUEUE;
uint32_t (*svcControlMemory)(uint32_t *outaddr, uint32_t *addr0, uint32_t *addr1, uint32_t size, uint32_t operation, uint32_t permissions) = (void *)FUNC_SVCCONTROLMEMORY;
int (*fopen)(uint32_t (*handle)[], short unsigned int *path, int flags) = (void *)FUNC_FOPEN;
int (*fread)(uint32_t (*handle)[], uint32_t *read, void *buffer, uint32_t size) = (void *)FUNC_FREAD;
int (*fwrite)(uint32_t (*handle)[], uint32_t *written, void *src, uint32_t size) = (void *)FUNC_FWRITE;

#ifdef ENTRY_SPIDER
int (*GX_SetTextureCopy)(void *input_buffer, void *output_buffer, uint32_t size, int in_x, int in_y, int out_x, int out_y, int flags) = (void *)FUNC_GX_SETTEXTURECOPY;
int (*svcSleepThread)(unsigned long long nanoseconds) = (void *)FUNC_SVCSLEEPTHREAD;
#endif
