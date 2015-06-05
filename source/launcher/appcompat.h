#ifndef __appcompat_h__
#define __appcompat_h__

#include <stdint.h>

// Functions
void (*memcpy)(void *dest, void *src, int len);
int (*GSPGPU_FlushDataCache)(void *address, uint32_t length);
void (*nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue)(void *arg1, void *arg2);
uint32_t (*svcControlMemory)(uint32_t *outaddr, uint32_t *addr0, uint32_t *addr1, uint32_t size, uint32_t operation, uint32_t permissions);
int (*fopen)(uint32_t (*handle)[], short unsigned int *path, int flags);
int (*fread)(uint32_t (*handle)[], uint32_t *read, void *buffer, uint32_t size);
int (*fwrite)(uint32_t (*handle)[], uint32_t *written, void *src, uint32_t size);

#ifdef ENTRY_SPIDER
int (*GX_SetTextureCopy)(void *input_buffer, void *output_buffer, uint32_t size, int in_x, int in_y, int out_x, int out_y, int flags);
int (*svcSleepThread)(unsigned long long nanoseconds);
#endif


#if defined(ENTRY_MSET_4x)
    #define FUNC_MEMCPY 0x001BFA60
    #define FUNC_GSPGPU_FLUSHDATACACHE 0x0013C5D4
    #define FUNC_NN__GXLOW__CTR__CMDREQQUEUETX__TRYENQUEUE 0x001AC924
    #define FUNC_SVCCONTROLMEMORY 0x001C3E24
    #define FUNC_FOPEN 0x001B82A8
    #define FUNC_FREAD 0x001B3954
    #define FUNC_FWRITE 0x001B3B50

    #define APP_GPUHANDLE (0x0027C580 + 0x58)

#elif defined(ENTRY_SPIDER_4x)
    #define FUNC_MEMCPY 0x0029BF60
    #define FUNC_GSPGPU_FLUSHDATACACHE 0x00344B84
    #define FUNC_NN__GXLOW__CTR__CMDREQQUEUETX__TRYENQUEUE 0x002CF3EC
    #define FUNC_SVCCONTROLMEMORY 0x002D6ADC
    #define FUNC_FOPEN 0x0025B0A4
    #define FUNC_FREAD 0x002FC8E4
    #define FUNC_FWRITE 0x00311D90

    #define FUNC_GX_SETTEXTURECOPY 0x002C62E4
    #define FUNC_SVCSLEEPTHREAD 0x002A513C

    #define APP_GPUHANDLE (0x003F54E8 + 0x58)

#elif defined(ENTRY_SPIDER_5x)
    #define FUNC_MEMCPY 0x00240B58
    #define FUNC_GSPGPU_FLUSHDATACACHE 0x001914FC
    #define FUNC_NN__GXLOW__CTR__CMDREQQUEUETX__TRYENQUEUE 0x0012BF4C
    #define FUNC_SVCCONTROLMEMORY 0x001431C0
    #define FUNC_FOPEN 0x0022FE44
    #define FUNC_FREAD 0x001686C0
    #define FUNC_FWRITE 0x00168748

    #define FUNC_GX_SETTEXTURECOPY 0x0011DD80
    #define FUNC_SVCSLEEPTHREAD 0x0010420C

    #define APP_GPUHANDLE (0x003D7C40 + 0x58)

#elif defined(ENTRY_SPIDER_9x)
    #define FUNC_MEMCPY 0x00240B50
    #define FUNC_GSPGPU_FLUSHDATACACHE 0x00191504
    #define FUNC_NN__GXLOW__CTR__CMDREQQUEUETX__TRYENQUEUE 0x0012BF04
    #define FUNC_SVCCONTROLMEMORY 0x001431A0
    #define FUNC_FOPEN 0x0022FE08
    #define FUNC_FREAD 0x001686DC
    #define FUNC_FWRITE 0x00168764

    #define FUNC_GX_SETTEXTURECOPY 0x0011DD48
    #define FUNC_SVCSLEEPTHREAD 0x0023FFE8

    #define APP_GPUHANDLE (0x003D7C40 + 0x58)
#endif


#define LAUNCHER_PATH L"Launcher.dat"

#if defined(ENTRY_MSET)
    // The usable area for this app
    #define APP_FCRAM_ADDR 0x14000000

    #define APP_CFW_OFFSET 0x400000
    #define APP_LAUNCHER_PATH (L"YS:/" LAUNCHER_PATH)

#elif defined(ENTRY_SPIDER)
    // The usable area for this app
    #define APP_FCRAM_ADDR 0x18400000

    #define APP_CFW_OFFSET 0x4410000
    #define APP_LAUNCHER_PATH (L"dmc:/" LAUNCHER_PATH)
#endif

// Locations in fcram
#define APP_CHECK_MEM (APP_FCRAM_ADDR + 0x1000)
#define APP_ARM11_BUFFER (APP_FCRAM_ADDR + 0x2000)
#define APP_MEM_HAX_MEM (APP_FCRAM_ADDR + 0x20000)
#define APP_FIRM_COMPAT (APP_FCRAM_ADDR + 0x30000)

#endif
