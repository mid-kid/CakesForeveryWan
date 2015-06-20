#include <stdint.h>
#include <draw.h>
#include "fs.h"
#include "menu.h"
#include "firm.h"
#include "patch.h"

// TODO: Put this in a nice place

void menu_main()
{
    while (1) {
        char *options[] = {"Boot CFW with sig patches, emunand and reboot",
                           "Boot CFW with sig patches and emunand",
                           "Boot CFW with sig patches"};
        int result = draw_menu("Main menu", 0, sizeof(options) / sizeof(char *), options);

        switch (result) {
            case 0:
                boot_cfw(2);
                break;
            case 1:
                boot_cfw(1);
                break;
            case 2:
                boot_cfw(0);
                break;
        }
    }
}

void main()
{
    draw_init((uint32_t *)0x23FFFE00);
    mount_sd();
    menu_main();
}
