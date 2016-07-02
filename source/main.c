#include <stdint.h>
#include <stdlib.h>
#include "patch.h"
#include "menu.h"
#include "memfuncs.h"
#include "config.h"
#include "firm.h"
#include "draw.h"
#include "fs.h"
#include "hid.h"
#include "fcram.h"
#include "paths.h"
#include "headers.h"
#include "fatfs/sdmmc/sdmmc.h"
#include "external/i2c.h"

#define MAX_OPTIONS 9

void menu_select_patches()
{
    #if MAX_CAKES > MAX_SELECTED_OPTIONS
    #error "This function needs MAX_CAKES to be <= MAX_SELECTED_OPTIONS"
    #endif

    if (cake_count <= 0) {
        draw_message("No cakes loaded", "No cakes have been loaded.\nPlease copy them to: " PATH_PATCHES);
        return;
    }

    char *options[cake_count];
    for (unsigned int i = 0; i < cake_count; i++) {
        options[i] = cake_list[i].description;
    }

    int *result = draw_selection_menu("Select your cakes", cake_count, options, cake_selected);

    patches_modified |= memcmp(cake_selected, result, sizeof(cake_selected));

    // The result location will be reused for other selection menus, so we memcpy it.
    memcpy(cake_selected, result, cake_count * sizeof(int));
}

void menu_toggle()
{
    char *options[] = {"Enable autoboot (Press L to enter the menu)",
                       "Force saving patched firmware",
                       "Silence debug output"};
    int preselected[] = {config->autoboot_enabled,
                         save_firm,
                         config->silent_boot};

    int *result = draw_selection_menu("Toggleable options", sizeof(options) / sizeof(char *),
                                      options, preselected);

    // Apply the options
    config->autoboot_enabled = result[0];
    save_firm = result[1];
    config->silent_boot = result[2];  // This doesn't change patches.
    patches_modified |= preselected[0] ? 0 : result[0];
    patches_modified |= preselected[1] ? 0 : result[1];
}

void menu_emunand()
{
    char emunands[MAX_OPTIONS][0x20];  // We have a max size for the strings...
    char unnamed[] = "emuNAND #";

    uint32_t gap;
    if (getMMCDevice(0)->total_size > 0x200000) {
        gap = 0x400000;
    } else {
        gap = 0x200000;
    }

    // Scan for available emuNANDS. Assume they're placed right behind eachother.
    int count;
    for (count = 0; count <= MAX_OPTIONS; count++) {
        if (get_emunand_offsets(count * gap, NULL, NULL) == 0) {
            if (sdmmc_sdcard_readsectors(count * gap, 1, fcram_temp) == 0 &&
                    memcmp(fcram_temp + 11, "NAME", 4) == 0) {
                memcpy(emunands[count], fcram_temp + 15, 0x1F);
                emunands[count][0x1F] = 0;
            } else {
                memcpy(emunands[count], unnamed, sizeof(unnamed));
                emunands[count][sizeof(unnamed) - 1] = '1' + count;
                emunands[count][sizeof(unnamed)] = 0;
            }
            continue;
        }

        break;
    }

    if (count <= 0) {
        print("Failed to find any emuNAND");
        draw_message("Failed to find any emuNAND",
                "There's 3 possible causes for this error:\n"
                " - You don't even have an emuNAND installed\n"
                " - Your SD card can't be read\n"
                " - You're using an unsupported emuNAND format");
        return;
    }

    // Make the pointer array
    char *options[count];
    for (int x = 0; x <= count; x++) options[x] = emunands[x];

    int result = draw_menu("Select emuNAND", 1, count, options);
    if (result == -1) return;

    config->emunand_location = result * gap;
    patches_modified = 1;
}

int menu_firms()
{
    char firms[MAX_OPTIONS][_MAX_LFN + 1], dirpath[] = PATH_FIRMWARE_DIR;

    int pathlen = strlen(dirpath);
    int count = find_file_pattern(firms, dirpath, pathlen, MAX_OPTIONS, "firmware*.bin");

    if (!count) {
        draw_loading("Failed to load FIRM", "Make sure the encrypted FIRM is\n  located in the firmware directory");
        return 1;
    }

    char *options[count];
    for (int x = 0; x <= count; x++) options[x] = firms[x];

    int result = draw_menu("Select firmware", 0, count, options);
    if (result == -1) return 0;

    memcpy(config->firm_path, firms[result], _MAX_LFN + 1);

    reload_native_firm();
    load_cakes_info(PATH_PATCHES);

    return 0;
}

void menu_more()
{
    while (1) {
        char *options[] = {"Toggleable options",
                           "Select emuNAND",
                           "Select firmware"};
        int result = draw_menu("More options", 1, sizeof(options) / sizeof(char *), options);
        
        switch (result) {
            case 0:
                menu_toggle();
                break;
            case 1:
                menu_emunand();
                break;
            case 2:
                menu_firms();
                break;
            case -1:
                return;
        }
    }
}

void version_info()
{
    int pos_y = draw_loading("Version info", "CakesFW version " CAKES_VERSION) + SPACING_VERT;
    int version_pos_x = SPACING_HORIZ * 21;

    draw_string(screen_top_left, "NATIVE_FIRM version:", 0, pos_y, COLOR_NEUTRAL);
    draw_string(screen_top_left, current_firm->version_string, version_pos_x, pos_y, COLOR_NEUTRAL);

    if (current_twl_firm) {
        pos_y += SPACING_VERT;

        draw_string(screen_top_left, "TWL_FIRM version:", 0, pos_y, COLOR_NEUTRAL);
        draw_string(screen_top_left, current_twl_firm->version_string, version_pos_x, pos_y, COLOR_NEUTRAL);
    }

    if (current_agb_firm) {
        pos_y += SPACING_VERT;

        draw_string(screen_top_left, "AGB_FIRM version:", 0, pos_y, COLOR_NEUTRAL);
        draw_string(screen_top_left, current_agb_firm->version_string, version_pos_x, pos_y, COLOR_NEUTRAL);
    }

    draw_string(screen_top_left, "Press B to return", 0, pos_y + 20, COLOR_SELECTED);
    while (1) {
        uint16_t key = wait_key();

        if (key == (key_released | key_b)) {
            return;
        }
    }
}

void menu_main()
{
    while (1) {
        char *options[] = {"Boot CFW",
                           "Select Patches",
                           "More options...",
                           "Version info",
                           "Power off"};
        int result = draw_menu("CakesFW " CAKES_VERSION, 0, sizeof(options) / sizeof(char *), options);

        switch (result) {
            case 0:
                save_config();
                boot_cfw();
                break;
            case 1:
                menu_select_patches();
                break;
            case 2:
                menu_more();
                break;
            case 3:
                version_info();
                break;
            case 4:
                i2cWriteRegister(I2C_DEV_MCU, 0x20, 1);
                while(1);  // Won't break out of this one >:D
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

        if (read_file(firm_loc, PATH_PATCHED_FIRMWARE, FCRAM_SPACING) != 0 ||
                firm_loc->magic != FIRM_MAGIC) {
            print("Failed to load patched FIRM");
            draw_message("Failed to load patched FIRM", "The option to autoboot was selected,\n  but no valid FIRM could be found at:\n  " PATH_PATCHED_FIRMWARE);
        } else {
            if (read_file(memory_loc, PATH_MEMORY, FCRAM_SPACING) != 0) {
                print("Failed to load memory patches");
                draw_message("Failed to load memory patches", "The option to autoboot was selected,\n  but no valid memory patches could be found at:\n  " PATH_MEMORY);
            } else {
                if (config->firm_console == console_n3ds && config->firm_ver > 0x0F) {
                    slot0x11key96_init();
                }

                // boot_firm() requires current_firm->console and current_firm->version.
                struct firm_signature config_firm = {.console = config->firm_console,
                                                     .version = config->firm_ver};
                current_firm = &config_firm;

                boot_firm();
            }
        }
    }

    if (load_firms() != 0) {
        if(menu_firms() !=0) return;
    }

    print("Loading cakes");
    if (load_cakes_info(PATH_PATCHES) != 0) {
        draw_loading("Failed to read some cakes", "Make sure your cakes are up to date\n  and your SD card can be read correctly");
        return;
    }

    load_config_cakes();

    menu_main();
}
