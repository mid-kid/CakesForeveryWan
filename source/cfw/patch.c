#include "patch.h"

#include <stdint.h>
#include <draw.h>
#include <memfuncs.h>
#include "fs.h"
#include "menu.h"
#include "firm.h"
#include "fatfs/sdmmc/sdmmc.h"

struct patch_header {
    uint8_t count: 8;
    uint8_t patches_offset: 8;
    char name;
} __attribute__((packed));

struct patch {
    void *address;
    void *offset;
    uint32_t size;
    uint8_t options: 8;
} __attribute__((packed));

enum patch_options {
    patch_option_keyx = 0b00000001,
    patch_option_emunand = 0b00000010,
    patch_option_save = 0b00000100
};

static struct patch_header *firm_patch_temp = (struct patch_header *)0x24200000;
static void *temp = (void *)0x24210000;

static const int nand_size_toshiba = 0x1D7800;
static const int nand_size_samsung = 0x1DD000;
static const uint32_t ncsd_magic = 0x4453434E;  // Hex for "NCSD". For easy comparing.

void *memsearch(void *start_pos, void *search, uint32_t size, uint32_t size_search)
{
    // Searching backwards, since most of the stuff we'll search with this are near the end.
    for (void *pos = start_pos + size - size_search; pos >= start_pos; pos--) {
        if (memcmp(pos, search, size_search) == 0) {
            return pos;
        }
    }

    return (void *)0;
}

int patch_options(void *address, uint32_t size, uint8_t options) {
    if (options & patch_option_keyx) {
        print("Patch option: Adding keyX");

        if (read_file(temp, "/slot0x25keyX.bin", 16) != 0) {
            print("Failed to load keyX");
            draw_message("Failed to load keyX", "Make sure the keyX is\n  located at /slot0x25keyX.bin");
            return 1;
        }

        void *pos = memsearch(address, "slot0x25keyXhere", size, 16);
        if (pos) {
            memcpy32(pos, temp, 16);
        } else {
            print("I don't know where to add keyX.\n  Ignoring...");
        }
    }
    if (options & patch_option_emunand) {
        print("Patch option: Setting emuNAND offsets");

        uint32_t offset = 0;
        uint32_t header = 0;

        if (sdmmc_sdcard_readsectors(1, 1, temp) == 0) {
            if (*(uint32_t *)(temp + 0x100) == (uint32_t)ncsd_magic) {
                print("emuNAND detected: redNAND");
                offset = 1;
                header = 1;
            }
        }

        if (sdmmc_sdcard_readsectors(nand_size_toshiba, 1, temp) == 0) {
            if (*(uint32_t *)(temp + 0x100) == (uint32_t)ncsd_magic) {
                print("emuNAND detected: Toshiba GW");
                offset = 0;
                header = nand_size_toshiba;
            }
        }

        if (sdmmc_sdcard_readsectors(nand_size_samsung, 1, temp) == 0) {
            if (*(uint32_t *)(temp + 0x100) == (uint32_t)ncsd_magic) {
                print("emuNAND detected: Samsung GW");
                offset = 0;
                header = nand_size_samsung;
            }
        }

        if (!header) {
            print("Failed to get the emunand offsets");
            draw_message("Failed to get the emunand offsets",
                    "There's 3 possible causes for this error:\n"
                    " - You don't even have an emuNAND installed\n"
                    " - You're SD card can't be read\n"
                    " - You're using an unsupported emuNAND format");
            return 1;
        }

        uint32_t *pos_offset = memsearch(address, "NAND", size, 4);
        uint32_t *pos_header = memsearch(address, "NCSD", size, 4);
        if (pos_offset && pos_header) {
            *pos_offset = offset;
            *pos_header = header;
        } else {
            print("I don't know where to set the offsets.\n  Ignoring...");
        }
    }
    if (options & patch_option_save) {
        print("Patch option: Save firm");

        save_firm = 1;

        // This next stuff is to "convert" the path to a short wchar.
        char *offset = address + size;
        memcpy(offset, "s\0d\0m\0c\0:", 10);
        offset += 10;

        for (int x = 0; x < strlen(save_path); x++) {
            *offset++ = save_path[x];
            *offset++ = 0;
        }

        memset(offset, 0, 4);
    }

    return 0;
}

int patch_firm(char *filename)
{
    if (read_file(firm_patch_temp, filename, 0x10000) != 0) {
        print("Failed to load patch");
        draw_message("Failed to load patch", "Please make sure all the patches you want\n  to apply actually exist on the SD card.");
        return 1;
    }
    print("Applying patch:");
    print(&firm_patch_temp->name);

    struct patch *patches = (void *)firm_patch_temp + firm_patch_temp->patches_offset;
    struct firm_section *sections = firm_loc + 0x40;
    struct firm_section process9;

    // TODO: Don't hard-code the offsets, use proper headers 'n stuff.
    process9.offset = (void *)0x7C700;
    process9.address = *(void **)(firm_loc + 0x7BD10);
    process9.size = *(uint32_t *)(firm_loc + 0x7C50C);

    // For some reason, patch 0 and 1 are the only ones not in Process9

    for (uint8_t i = 0; i < firm_patch_temp->count; i++) {
        if (patches[i].address >= process9.address &&
              patches[i].address < process9.address + process9.size) {
            void *offset = firm_loc + (uintptr_t)(process9.offset + (patches[i].address - process9.address));
            memcpy(offset, (uintptr_t)firm_patch_temp + patches[i].offset, patches[i].size);

            if (patches[i].options) {
                if (patch_options(offset, patches[i].size, patches[i].options) != 0) {
                    return 1;
                }
            }

            continue;
        }

        for (int x = 0; x < 3; x++) {
            if (patches[i].address >= sections[x].address &&
                  patches[i].address < sections[x].address + sections[x].size) {
                void *offset = firm_loc + (uintptr_t)(sections[x].offset + (patches[i].address - sections[x].address));
                memcpy(offset, (uintptr_t)firm_patch_temp + patches[i].offset, patches[i].size);

                if (patches[i].options) {
                    if (patch_options(offset, patches[i].size, patches[i].options) != 0) {
                        return 1;
                    }
                }

                break;
            }
        }
    }

    return 0;
}

int patch_firm_all(int patch_level)
{
    if (patch_firm("/cakes/patches/signatures.cake") != 0) return 1;

    if (patch_level >= 1) {
        if (patch_firm("/cakes/patches/emunand.cake") != 0) return 1;

        if (patch_level >= 2) {
            if (patch_firm("/cakes/patches/reboot.cake") != 0) return 1;
        }
    }

    return 0;
}
