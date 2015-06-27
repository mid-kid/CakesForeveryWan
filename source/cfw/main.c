#include <stdint.h>
#include <memfuncs.h>
#include <draw.h>
#include "fs.h"
#include "menu.h"
#include "firm.h"
#include "patch.h"
#include "fatfs/ff.h"

void menu_select_patches()
{
    #if MAX_CAKES > MAX_SELECTED_OPTIONS
    #error "This function needs MAX_CAKES to be <= MAX_SELECTED_OPTIONS"
    #endif

    char *options[cake_count];
    for (int i = 0; i < cake_count; i++) {
        options[i] = cake_list[i].description;
    }

    int *result = draw_selection_menu("Select your cakes", cake_count, options, cake_selected);
    memcpy(cake_selected, result, cake_count * sizeof(int));
}

void menu_main()
{
    while (1) {
        char *options[] = {"Boot CFW",
                           "Select Patches"};
        int result = draw_menu("CakesFW", 0, sizeof(options) / sizeof(char *), options);

        switch (result) {
            case 0:
                boot_cfw();
                break;
            case 1:
                menu_select_patches();
                break;
        }
    }
}

void main()
{
    draw_init((uint32_t *)0x23FFFE00);

    int rc;

    rc = mount_sd();
    if(rc == 0 && load_firm() == 0)
    {
        rc = load_cakes_info();
        menu_main();
    }
}
