#ifndef __firmcompat_h__
#define __firmcompat_h__

#include <stdint.h>

struct firmware_offsets {
    uint32_t kernel_patch_address;
    uint32_t reboot_patch_address;
    uint32_t reboot_func_address;
    uint32_t jump_table_address;
    uint32_t fcram_address;
    uint32_t func_patch_address;
    uint32_t func_patch_return;
    uint32_t pdn_regs;
    uint32_t pxi_regs;
};

static struct firmware_offsets *fw = (struct firmware_offsets *)0x14A00001;

int set_firmware_offsets();

#endif
