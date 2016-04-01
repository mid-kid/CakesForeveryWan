#include "hid.h"

#include <stdint.h>

volatile uint16_t *const hid_regs = (volatile uint16_t *)0x10146000;
static uint16_t old_status = 0;

uint16_t wait_key()
{
    if (old_status == 0) {
        old_status = *hid_regs;
    }

    uint16_t new_status;
    do {
        while ((new_status = *hid_regs) == old_status);

        // Simple debounce. Makes sure the key is really pressed.
        int debounce = 0xFEE7;
        while (debounce--) {
            if (new_status != *hid_regs) {
                old_status = new_status;
                break;
            }
        }

    } while (old_status == new_status);

    uint16_t return_status = new_status ^ old_status;
    if (new_status > old_status) {
        return_status |= key_released;
    } else {
        return_status |= key_pressed;
    }

    old_status = new_status;
    return return_status;
}
