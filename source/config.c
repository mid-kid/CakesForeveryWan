#include "config.h"

#include "memfuncs.h"
#include "fs.h"
#include "draw.h"
#include "patch.h"
#include "menu.h"
#include "firm.h"
#include "fcram.h"
#include "paths.h"

static unsigned int config_ver = 5;

struct config_file *config = (struct config_file *)FCRAM_CONFIG;
int patches_modified = 0;

void load_config()
{
    // Make sure we don't get random values if the config file doesn't load
    memset(config, 0, sizeof(struct config_file));

    if (read_file(config, PATH_CONFIG, 0x100000) != 0) {
        print("Failed to load the config.\n  Starting from scratch.");
        memcpy(config->firm_path, PATH_FIRMWARE, strlen(PATH_FIRMWARE));
        patches_modified = 1;
        return;
    }

    if (config->config_ver != config_ver) {
        print("Invalid config version\n  Starting from scratch");
        memset(config, 0, sizeof(struct config_file));
        memcpy(config->firm_path, PATH_FIRMWARE, strlen(PATH_FIRMWARE));
        patches_modified = 1;
        return;
    }

    if (!config->firm_path[0]) {
        print("Firmware config not found.\n Defaulting to firmware.bin");
        strncpy(config->firm_path, PATH_FIRMWARE, strlen(PATH_FIRMWARE));
    }

    print("Loaded config file");
}

void load_config_cakes()
{
    if (patches_modified) return;

    // TODO: If we get more options, maybe we should keep them when swapping firms.
    if (config->firm_ver != current_firm->version || config->firm_console != current_firm->console) {
        print("Config was for another firm version.\n"
              "  Starting from scratch.");

        memset(config, 0, sizeof(struct config_file));
        patches_modified = 1;
        return;
    }

    // Someone may try to be funny and make the count too big.
    const unsigned int max_cakes = (FCRAM_SPACING - sizeof(struct config_file)) /
                                   sizeof(config->cake_list[0]);

    for (unsigned int x = 0; x < cake_count; x++) {
        for (unsigned int y = 0; y < config->cake_count && y < max_cakes; y++) {
            if (strncmp(cake_list[x].path, config->cake_list[y],
                        sizeof(config->cake_list[0])) == 0) {
                cake_selected[x] = 1;
            }
        }
    }

    print("Loaded selected cakes");
}

void save_config()
{
    // This is the maximum size the cake_list could be
    unsigned int cake_list_size = sizeof(config->cake_list[0]) * cake_count;

    // Boundary checking.
    if (cake_list_size > FCRAM_SPACING - sizeof(struct config_file)) {
        cake_list_size = FCRAM_SPACING - sizeof(struct config_file);
    }

    // Set config file version
    config->config_ver = config_ver;

    // Set the firmware version and console
    config->firm_ver = current_firm->version;
    config->firm_console = current_firm->console;

    // Clean the memory area. We don't want to dump random bytes.
    memset(config->cake_list, 0, cake_list_size);

    // More boundary checking.
    const unsigned int max_cakes = (FCRAM_SPACING - sizeof(struct config_file)) /
                                   sizeof(config->cake_list[0]);

    config->cake_count = 0;
    for (unsigned int i = 0; i < cake_count && i < max_cakes; i++) {
        if (cake_selected[i]) {
            // This saves the full path...
            strncpy(config->cake_list[config->cake_count++],
                    cake_list[i].path, sizeof(config->cake_list[0]));
        }
    }

    int config_size = sizeof(struct config_file) +
                      sizeof(config->cake_list[0]) * config->cake_count;
    if (write_file(config, PATH_CONFIG, config_size) != 0) {
        print("Failed to write the config file");
        draw_message("Failed to write the config file",
            "CakesFW will continue working normally.\n"
            "However, any changes to the configuration you may have applied\n"
            "  will not be saved.");
        return;
    }

    print("Saved config file");
}
