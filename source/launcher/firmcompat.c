#include "firmcompat.h"

#include <stdint.h>
#include "draw.h"

int set_firmware_offsets()
{
    uint32_t kernel_version = *(uint32_t *)0x1FF80000;

    // Offsets taken from bootstrap
    switch (kernel_version) {
        case 0x02220000:  // 2.34-0 4.1.0
            fw->kernel_patch_address = 0xEFF83C97;
            fw->reboot_patch_address = 0xEFFF497C;
            fw->reboot_func_address = 0xFFF748C4;
            fw->jump_table_address = 0xEFFF4C80;
            fw->fcram_address = 0xF0000000;
            fw->func_patch_address = 0xEFFE4DD4;
            fw->func_patch_return = 0xFFF84DDC;
            fw->pdn_regs = 0xFFFD0000;
            fw->pxi_regs = 0xFFFD2000;
            fw->gpu_regs = 0xFFFCE000;
            break;

        case 0x02230600:  // 2.35-6 5.0.0
            fw->kernel_patch_address = 0xEFF8372F;
            fw->reboot_patch_address = 0xEFFF4978;
            fw->reboot_func_address = 0xFFF64B94;
            fw->jump_table_address = 0xEFFF4C80;
            fw->fcram_address = 0xF0000000;
            fw->func_patch_address = 0xEFFE55BC;
            fw->func_patch_return = 0xFFF765C4;
            fw->pdn_regs = 0xFFFD0000;
            fw->pxi_regs = 0xFFFD2000;
            fw->gpu_regs = 0xFFFCE000;
            break;

        case 0x02240000:  // 2.36-0 5.1.0
            fw->reboot_func_address = 0xFFF64B90;
            fw->func_patch_address = 0xEFFE55B8;
            fw->func_patch_return = 0xFFF765C0;
            fw->pdn_regs = 0xFFFD0000;
            fw->pxi_regs = 0xFFFD2000;
            fw->kernel_patch_address = 0xEFF8372B;
            fw->reboot_patch_address = 0xEFFF4978;
            fw->jump_table_address = 0xEFFF4C80;
            fw->fcram_address = 0xF0000000;
            fw->gpu_regs = 0xFFFCE000;
            break;

        case 0x02250000:  // 2.37-0 6.0.0
        case 0x02260000:  // 2.38-0 6.1.0
            fw->kernel_patch_address = 0xEFF8372B;
            fw->reboot_patch_address = 0xEFFF4978;
            fw->reboot_func_address = 0xFFF64A78;
            fw->func_patch_address = 0xEFFE5AE8;
            fw->func_patch_return = 0xFFF76AF0;
            fw->pdn_regs = 0xFFFD0000;
            fw->pxi_regs = 0xFFFD2000;
            fw->jump_table_address = 0xEFFF4C80;
            fw->fcram_address = 0xF0000000;
            fw->gpu_regs = 0xFFFCE000;
            break;

        case 0x02270400:  // 2.39-4 7.0.0
            fw->kernel_patch_address = 0xEFF8372F;
            fw->reboot_patch_address = 0xEFFF4978;
            fw->reboot_func_address = 0xFFF64AB0;
            fw->jump_table_address = 0xEFFF4C80;
            fw->fcram_address = 0xF0000000;
            fw->func_patch_address = 0xEFFE5B34;
            fw->func_patch_return = 0xFFF76B3C;
            fw->pdn_regs = 0xFFFD0000;
            fw->pxi_regs = 0xFFFD2000;
            fw->gpu_regs = 0xFFFCE000;
            break;

        case 0x02280000:  // 2.40-0 7.2.0
            fw->kernel_patch_address = 0xEFF8372B;
            fw->reboot_patch_address = 0xEFFF4974;
            fw->reboot_func_address = 0xFFF54BAC;
            fw->jump_table_address = 0xEFFF4C80;
            fw->fcram_address = 0xF0000000;
            fw->func_patch_address = 0xEFFE5B30;
            fw->func_patch_return = 0xFFF76B38;
            fw->pdn_regs = 0xFFFD0000;
            fw->pxi_regs = 0xFFFD2000;
            fw->gpu_regs = 0xFFFBC000;
            break;

        case 0x022C0600:  // 2.44-6 8.0.0
            fw->kernel_patch_address = 0xDFF83767;
            fw->reboot_patch_address = 0xDFFF4974;
            fw->reboot_func_address = 0xFFF54BAC;
            fw->jump_table_address = 0xDFFF4C80;
            fw->fcram_address = 0xE0000000;
            fw->func_patch_address = 0xDFFE4F28;
            fw->func_patch_return = 0xFFF66F30;
            fw->pdn_regs = 0xFFFBE000;
            fw->pxi_regs = 0xFFFC0000;
            fw->gpu_regs = 0xFFFC6000;
            break;

        case 0x022E0000:  // 2.26-0 9.0.0
            fw->kernel_patch_address = 0xDFF83837;
            fw->reboot_patch_address = 0xDFFF4974;
            fw->reboot_func_address = 0xFFF151C0;
            fw->jump_table_address = 0xDFFF4C80;
            fw->fcram_address = 0xE0000000;
            fw->func_patch_address = 0xDFFE59D0;
            fw->func_patch_return = 0xFFF279D8;
            fw->pdn_regs = 0xFFFC2000;
            fw->pxi_regs = 0xFFFC4000;
            //fw->gpu_regs = 0xFFFC6000;
            fw->gpu_regs = 0xFFFC0000;
            break;

        default:
            print("Unsupported/Unrecognized kernel version");
            return 1;
    }

    return 0;
}
