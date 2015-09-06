#include "patch.h"
#include "menu.h"
#include "memfuncs.h"
#include "config.h"
#include "firm.h"
#include "draw.h"
#include "fs.h"
#include "hid.h"
#include "paths.h"

void menu_select_patches()
{
    #if MAX_CAKES > MAX_SELECTED_OPTIONS
    #error "This function needs MAX_CAKES to be <= MAX_SELECTED_OPTIONS"
    #endif

    char *options[cake_count];
    for (unsigned int i = 0; i < cake_count; i++) {
        options[i] = cake_list[i].description;
    }

    int *result = draw_selection_menu("Select your cakes", cake_count, options, cake_selected);

    patches_modified |= memcmp(cake_selected, result, sizeof(cake_selected));

    // The result location will be reused for other selection menus, so we memcpy it.
    memcpy(cake_selected, result, cake_count * sizeof(int));
}

void menu_config()
{
    char *options[] = {"Enable autoboot (Press \"L\" to enter the menu)"};
    int preselected[] = {config->autoboot_enabled};

    int *result = draw_selection_menu("Configuration", sizeof(options) / sizeof(char *),
                                      options, preselected);

    // Apply the options
    config->autoboot_enabled = result[0];
}

void menu_main()
{
    while (1) {
        char *options[] = {"Boot CFW",
                           "Select Patches",
                           "Configuration"};
        int result = draw_menu("CakesFW", 0, sizeof(options) / sizeof(char *), options);

        switch (result) {
            case 0:
                save_config();
                boot_cfw();
                break;
            case 1:
                menu_select_patches();
                break;
            case 2:
                menu_config();
                break;
        }
    }
}

void main()
{
    clear_screens();

    if(mount_sd() != 0) {
        draw_loading("Failed to mount SD", "Make sure your SD card can be read correctly");
        return;
    }

    load_config();

    // If the L button isn't pressed, autoboot.
    if (config->autoboot_enabled && *hid_regs ^ 0xFFF ^ key_l) {
        print("Autobooting...");

        if (read_file(firm_loc, PATH_PATCHED_FIRMWARE, 0) != 0 ||
                firm_loc->magic != FIRM_MAGIC) {
            print("Failed to load patched FIRM");
            draw_message("Failed to load patched FIRM", "The option to autoboot was selected,\n  but no valid FIRM could be found at\n  " PATH_PATCHED_FIRMWARE);
        } else {
            // boot_firm() requires current_firm->console.
            struct firm_signature config_firm = {.console = config->firm_console}; 
            current_firm = &config_firm;

            boot_firm();
        }
    }

    // This function already correctly draws error messages
    if (load_firm() != 0) return;

    if (load_cakes_info(PATH_PATCHES) != 0) {
        draw_loading("Failed to read some cakes", "Make sure your cakes are up to date\n  and your SD card can be read correctly");
        return;
    }

    load_config_cakes();

    menu_main();
}
