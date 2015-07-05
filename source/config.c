#include "config.h"

#include "memfuncs.h"
#include "fs.h"
#include "draw.h"
#include "patch.h"
#include "menu.h"

struct config_file *config = (struct config_file *)0x24400000;

void load_config()
{
    // Make sure we don't get random values if the config file doesn't load
    memset(config, 0, sizeof(struct config_file));

    if (read_file(config, "/cakes/config.dat", 0x100000) != 0) {
        print("Failed to load the config file.\n  Starting from scratch.");
        return;
    }

    // Someone may try to be funny and make the count too big.
    const int max_autoboot = (0x100000 - sizeof(struct config_file)) /
                              sizeof(config->autoboot_list[0]);

    for (int x = 0; x < cake_count; x++) {
        for (int y = 0; y < config->autoboot_count && y < max_autoboot; y++) {
            if (strncmp(cake_list[x].path, config->autoboot_list[y],
                        sizeof(config->autoboot_list[0])) == 0) {
                cake_selected[x] = 1;
            }
        }
    }
    
    print("Loaded config file");
}

void save_config()
{
    // This is the maximum size our file could be.
    int max_size = sizeof(struct config_file) + sizeof(config->autoboot_list[0]) * cake_count;

    // Boundary checking. We expect 0x24500000 to be used.
    if (max_size > 0x100000) {
        max_size = 0x100000;
    }

    // Clean the memory area. We don't want to dump random bytes.
    memset32(config, 0, max_size);

    // More boundary checking. Make absolutely sure we don't write on 0x24500000.
    const int max_autoboot = (0x100000 - sizeof(struct config_file)) /
                              sizeof(config->autoboot_list[0]);

    config->autoboot_count = 0;
    for (int i = 0; i < cake_count && i < max_autoboot; i++) {
        if (cake_selected[i]) {
            // This saves the full path...
            strncpy(config->autoboot_list[config->autoboot_count],
                    cake_list[i].path, sizeof(config->autoboot_list[0]));
            config->autoboot_count++;
        }
    }

    int config_size = sizeof(struct config_file) +
                      sizeof(config->autoboot_list[0]) * config->autoboot_count;
    if (write_file(config, "/cakes/config.dat", config_size) != 0) {
        print("Failed to write the config file");
        draw_message("Failed to write the config file",
            "CakesFW will continue working normally.\n"
            "However, any changes to the configuration you may have applied\n"
            "  will not be saved.");
        return;
    }

    print("Saved config file");
}
