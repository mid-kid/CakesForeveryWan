#include "firmlaunchax.h"

#include <cfw_bin.h>
#include <arm9hax_bin.h>
#include <stdint.h>
#include "firmcompat.h"
#include "arm11_tools.h"
#include "../mset/draw.h"

void firmlaunch_arm9hax()
{
    invalidate_data_cache();
    invalidate_instruction_cache();
    print("Invalidated instruction and data cache");

    uint32_t code_offset = 0x3F00000;
    asm_memcpy((void *)(fw->fcram_address + code_offset), cfw_bin, cfw_bin_size);
    print("Copied arm9 code");

    asm_memcpy((void *)fw->jump_table_address, arm9hax_bin, arm9hax_bin_size);
    print("Copied jump table");

    int jt_func_patch_return = 0xCC;
    int jt_pdn_regs = 0xC4;
    int jt_pxi_regs = 0x1D8;

    *(uint32_t *)(fw->jump_table_address + jt_func_patch_return) = fw->func_patch_return;
    *(uint32_t *)(fw->jump_table_address + jt_pdn_regs) = fw->pdn_regs;
    *(uint32_t *)(fw->jump_table_address + jt_pxi_regs) = fw->pxi_regs;
    print("Written firmware specific offsets");

    *(uint32_t *)fw->func_patch_address = 0xE51FF004;
    *(uint32_t *)(fw->func_patch_address + 4) = 0xFFFF0C80;
    *(uint32_t *)fw->reboot_patch_address = 0xE51FF004;
    *(uint32_t *)(fw->reboot_patch_address + 4) = 0x1FFF4C80+4;
    print("Patched arm11 functions");

    invalidate_data_cache();
    print("Invalidated data cache");

    void (*reboot_func)(int, int, int, int) = (void *)fw->reboot_func_address;

    print("Triggering reboot");
    reboot_func(0, 0, 2, 0);

    // For some reason, the last instruction will be repeated without this.
    while (1) {};
}
