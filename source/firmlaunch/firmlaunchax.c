#include "firmlaunchax.h"

#include <cfw_bin.h>
#include <arm9hax_bin.h>
#include "arm11_tools.h"
#include "../mset/draw.h"

void firmlaunch_arm9hax()
{
    invalidate_data_cache();
    invalidate_instruction_cache();
    print("Invalidated instruction and data cache");

    int fcram_address = 0xF0000000;
    int code_offset = 0x3F00000;
    asm_memcpy((void *)(fcram_address + code_offset), cfw_bin, cfw_bin_size);
    print("Copied arm9 code");

    int jump_table_addr = 0xEFFF4C80;
    asm_memcpy((void *)jump_table_addr, arm9hax_bin, arm9hax_bin_size);
    print("Copied jump table");

    int jt_func_patch_return_loc = 0xCC;
    int jt_pdn_regs = 0xC4;
    int jt_pxi_regs = 0x1D8;
    int func_patch_return_loc = 0xFFF84DDC;
    int pdn_regs = 0xFFFD0000;
    int pxi_regs = 0xFFFD2000;

    *(int *)(jump_table_addr + jt_func_patch_return_loc) = func_patch_return_loc;
    *(int *)(jump_table_addr + jt_pdn_regs) = pdn_regs;
    *(int *)(jump_table_addr + jt_pxi_regs) = pxi_regs;
    print("Written firmware specific offsets");

    int func_patch_addr = 0xEFFE4DD4;
    int reboot_patch_addr = 0xEFFF497C;

    *(int *)func_patch_addr = 0xE51FF004;
    *(int *)(func_patch_addr + 4) = 0xFFFF0C80;
    *(int *)reboot_patch_addr = 0xE51FF004;
    *(int *)(reboot_patch_addr + 4) = 0x1FFF4C80+4;  // TODO: This is firmware-specific
    print("Patched arm11 functions");

    invalidate_data_cache();
    print("Invalidated data cache");

    void (*reboot_func)(int a, int b, int c, int d) = (void *)0xFFF748C4;

    print("Triggering reboot");
    reboot_func(0, 0, 2, 0);

    // For some reason, the last instruction will be repeated without this.
    while (1) {};
}
