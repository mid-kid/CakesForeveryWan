#include "patch.h"

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "headers.h"
#include "firm.h"

#ifndef STANDALONE
#include "draw.h"
#include "memfuncs.h"
#include "fs.h"
#include "menu.h"
#include "fcram.h"
#include "paths.h"
#include "crypto.h"
#include "config.h"
#include "fatfs/ff.h"
#include "fatfs/sdmmc/sdmmc.h"
#else
#include <string.h>
#include <stdio.h>
#define print(string) puts(string)
#define draw_message(title, description) printf("-- %s:\n%s\n", title, description)
#endif

#define FORMAT_VERSION 1

enum types {
    TYPE_FIRM,
    TYPE_MEMORY,
    TYPE_USERLAND
};

enum firm_types {
    NATIVE_FIRM,
    TWL_FIRM,
    AGB_FIRM
};

enum patch_options {
    patch_option_keyx = 0b00000001,
    patch_option_emunand = 0b00000010,
    patch_option_save = 0b00000100
};

struct cake_header {
    uint8_t version;
    uint8_t patch_count;
    uint8_t patches_offset;
    char description[];
} __attribute__((packed));

struct patch {
    uint8_t type;
    union {
        struct {
            uint16_t firm_type;
            uint16_t memory_id;
            uint32_t memory_offset;
        } __attribute__((packed));
        char name[8];
    };
    uint32_t offset;
    uint32_t size;
    uint8_t options;
    uint8_t version_count;
    uint32_t versions_offset;
    uint8_t variable_count;
    uint32_t variables_offset;
} __attribute__((packed));

struct patch_versions {
    union {
        struct {
            uint16_t firm_version;
            uint16_t console;
        } __attribute__((packed));
        uint32_t version;
    };
    uint32_t memory_offset;
    uint32_t values_offset;
} __attribute__((packed));

#ifndef STANDALONE
struct cake_info *cake_list = (struct cake_info *)FCRAM_CAKE_LIST;
unsigned int cake_count = 0;

static struct cake_header *firm_patch_temp = (struct cake_header *)FCRAM_FIRM_PATCH_TEMP;

void *memsearch(void *start_pos, void *search, uint32_t size, uint32_t size_search)
{
    // Searching backwards, since most of the stuff we'll search with this are near the end.
    for (void *pos = start_pos + size - size_search; pos >= start_pos; pos--) {
        if (memcmp(pos, search, size_search) == 0) {
            return pos;
        }
    }

    return NULL;
}

int get_emunand_offsets(uint32_t location, uint32_t *offset, uint32_t *header)
{
    if (sdmmc_sdcard_readsectors(location + 1, 1, fcram_temp) == 0) {
        if (*(uint32_t *)(fcram_temp + 0x100) == NCSD_MAGIC) {
            if (offset && header) {
                print("emuNAND detected: redNAND");
                *offset = location + 1;
                *header = location + 1;
            }
            return 0;
        }
    }

    uint32_t nand_size = getMMCDevice(0)->total_size;

    if (sdmmc_sdcard_readsectors(location + nand_size, 1, fcram_temp) == 0) {
        if (*(uint32_t *)(fcram_temp + 0x100) == NCSD_MAGIC) {
            if (offset && header) {
                print("emuNAND detected: Gateway");
                *offset = location;
                *header = location + nand_size;
            }
            return 0;
        }
    }

    return 1;
}

int patch_options(void *address, uint32_t size, uint8_t options, enum firm_types type)
{
    if (options & patch_option_keyx) {
        print("Patch option: Adding keyX");

        if (read_file(fcram_temp, PATH_SLOT0X25KEYX, AES_BLOCK_SIZE) != 0) {
            print("Failed to load keyX");
            draw_message("Failed to load keyX", "Make sure the keyX is\n  located at /slot0x25keyX.bin");
            return 1;
        }

        void *pos = memsearch(address, "slot0x25keyXhere", size, AES_BLOCK_SIZE);
        if (pos) {
            memcpy32(pos, fcram_temp, AES_BLOCK_SIZE);
        } else {
            print("I don't know where to add keyX.\n  Ignoring...");
        }
    }
    if (options & patch_option_emunand) {
        print("Patch option: Setting emuNAND offsets");

        uint32_t offset = 0;
        uint32_t header = 0;

        if (get_emunand_offsets(config->emunand_location, &offset, &header)) {
            print("Failed to get the emuNAND offsets");
            draw_message("Failed to get the emuNAND offsets",
                    "There's 3 possible causes for this error:\n"
                    " - You don't even have an emuNAND installed\n"
                    " - Your SD card can't be read\n"
                    " - You're using an unsupported emuNAND format");
            return 1;
        }

        uint32_t *pos_offset = memsearch(address, "NAND", size, 4);
        uint32_t *pos_header = memsearch(address, "NCSD", size, 4);
        if (pos_offset && pos_header) {
            *pos_offset = offset;
            *pos_header = header;
        } else {
            print("I don't know where to set the offsets.\n"
                  "  Ignoring...");
        }
    }
    if (options & patch_option_save && type == NATIVE_FIRM) {
        print("Patch option: Save firm");

        save_firm = 1;

        // This absolutely requires the -fshort-wchar option to be enabled.
        char *offset = address + size;
        memcpy(offset, L"sdmc:", 10);
        memcpy(offset + 10, L"" PATH_PATCHED_FIRMWARE, sizeof(PATH_PATCHED_FIRMWARE) * 2);
    }

    return 0;
}
#else
int patch_options()
{
    return 0;
}
#endif

int patch_firm(const void *_cake)
{
    struct cake_header *cake = (struct cake_header *)_cake;

    if (cake->version != FORMAT_VERSION) {
        print("Outdated cake or unknown version");
        draw_message("Outdated cake or unknown version", "This cake is either outdated or is of an unknown version of the format.");
        return 1;
    }

    print("Applying cake:");
    print(cake->description);

    struct patch *patches = (struct patch *)((uintptr_t)cake + cake->patches_offset);

    for (struct patch *patch = patches;
            patch < patches + cake->patch_count; patch++) {
        // Depending on the type, we have to use it in a different way
        if (patch->type == TYPE_FIRM) {
            // Process9 locations for all the different firms
            static firm_section_h native_process9;
            static int native_process9_init;
            static firm_section_h agb_process9;
            static int agb_process9_init;

            // Variables for the current firm
            firm_h *firm = NULL;
            struct firm_signature *firm_info = NULL;
            firm_section_h *process9 = NULL;
            int *process9_init = NULL;

            // Figure out which firm we have to patch
            switch (patch->firm_type) {
                case NATIVE_FIRM:
                    firm = firm_loc;
                    firm_info = current_firm;
                    process9 = &native_process9;
                    process9_init = &native_process9_init;
                    break;

                case AGB_FIRM:
                    firm = agb_firm_loc;
                    firm_info = current_agb_firm;
                    process9 = &agb_process9;
                    process9_init = &agb_process9_init;
                    break;

                case TWL_FIRM:
                default:
                    print("Unsupported FIRM type");
                    draw_message("Unsupported FIRM type", "This cake uses an unknown or unsupported FIRM type.");
                    return 1;
            }

            if (!firm) {
                print("FIRM not loaded");
                draw_message("FIRM not loaded", "The FIRM this cake tries to patch isn't loaded.\nPlease make sure it's installed correctly and is loaded.");
                return 1;
            }

            firm_section_h *firm_sections = firm->section;

            if (!*process9_init) {
                // Look for process9 in all sections
                for (firm_section_h *section = firm_sections;
                        section < firm_sections + 4; section++) {
                    if (section->address == 0) {
                        // Stop looping if the section address is null
                        break;
                    } else if (section->type != FIRM_TYPE_ARM9) {
                        // Process9 can only be found in arm9 sections
                        continue;
                    }

                    // Look for the string "Process9" in this section
                    for (uint32_t *arm9section = (uint32_t *)((uintptr_t)firm + section->offset);
                            arm9section < (uint32_t *)((uintptr_t)firm + section->offset + section->size);
                            arm9section++) {
                        if (arm9section[0] == 0x636F7250 && arm9section[1] == 0x39737365) {
                            ncch_h *ncch = (ncch_h *)((uintptr_t)arm9section - sizeof(ncch_h));
                            if (ncch->magic == NCCH_MAGIC) {
                                // Found Process9
                                ncch_ex_h *p9exheader = (ncch_ex_h *)(ncch + 1);
                                exefs_h *p9exefs = (exefs_h *)(p9exheader + 1);

                                process9->address = p9exheader->sci.textCodeSet.address;
                                process9->size = p9exefs->fileHeaders[0].size;
                                process9->offset = (uintptr_t)(p9exefs + 1) - (uintptr_t)firm;

                                (*process9_init)++;
                                goto found_process9;
                            }
                        }
                    }
                }
                print("Couldn't find Process9");
                draw_message("Couldn't find Process9", "Process9 couldn't be found on your FIRM. This is a bug.");
                return 1;
            }
found_process9:;

            void *patch_code = (void *)((uintptr_t)cake + patch->offset);
            struct patch_versions *versions = (struct patch_versions *)((uintptr_t)cake + patch->versions_offset);
            struct patch_versions *version = NULL;

            // Look for the correct patch version info
            for (struct patch_versions *patch_version = versions;
                    patch_version < versions + patch->version_count; patch_version++) {
                if (patch_version->console == firm_info->console &&
                        patch_version->firm_version == firm_info->version) {
                    version = patch_version;
                    break;
                }
            }

            if (!version) {
                print("Unsupported FIRM version");
                draw_message("Unsupported FIRM version", "This patch doesn't support the currently used FIRM version");
                return 1;
            }

            // Apply all the variables for this version
            uint32_t *variables = (uint32_t *)((uintptr_t)cake + patch->variables_offset);
            uint32_t *values = (uint32_t *)((uintptr_t)cake + version->values_offset);

            for (int x = 0; x < patch->variable_count; x++) {
                *(uint32_t *)(patch_code + variables[x]) = values[x];
            }

            // Look for the location in the FIRM to apply the patch
            int x;
            for (x = 0; x < 5; x++) {
                firm_section_h *section;

                // Try process9 before anything else
                if (x == 0) {
                    section = process9;
                } else {
                    section = &firm_sections[x - 1];

                    // Stop scanning at the end of the section list
                    if (section->address == 0) {
                        break;
                    }
                }

                if (version->memory_offset >= section->address &&
                        version->memory_offset < section->address + section->size) {
                    void *offset = (void *)((uintptr_t)firm + section->offset + (version->memory_offset - section->address));
                    memcpy(offset, (void *)((uintptr_t)cake + patch->offset), patch->size);

                    if (patch->options) {
                        if (patch_options(offset, patch->size, patch->options, patch->type) != 0) {
                            return 1;
                        }
                    }

                    break;
                }
            }

            if (x >= 5) {
                print("Failed to apply patch");
                draw_message("Failed to apply patch", "The location where the patch should be applied could not be found");
            }

        } else {
            print("Unsupported patch type");
            draw_message("Unsupported patch type", "This cake uses an unknown or unsupported patch type.");
            return 1;
        }
    }

    return 0;
}

#ifndef STANDALONE
int patch_firm_all()
{
    for (unsigned int i = 0; i < cake_count; i++) {
        if (cake_selected[i]) {
            if (read_file(firm_patch_temp, cake_list[i].path, FCRAM_SPACING) != 0) {
                print("Failed to load patch");
                draw_message("Failed to load patch", "Please make sure all the patches you want\n  to apply actually exist on the SD card.");
                return 1;
            }
            if (patch_firm(firm_patch_temp)) return 1;
        }
    }

    return 0;
}

int load_cakes_info(const char *dirpath)
{
    FRESULT fr;
    DIR dir;
    FILINFO fno;
    FIL handle;

    const int pathlen = strlen(dirpath);

    static char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);

    fr = f_opendir(&dir, dirpath);
    if (fr != FR_OK) goto error;

    static_assert(MAX_CAKES < 0x100000 / sizeof(struct cake_info),
                  "This function will overflow it's buffer");

    while (cake_count < MAX_CAKES) {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK) {
            goto error;
        } else if (fno.fname[0] == 0) {
            break;
        }

        char *fn = *fno.lfname ? fno.lfname : fno.fname;

        // Build the path string
        memcpy(cake_list[cake_count].path, dirpath, pathlen);
        cake_list[cake_count].path[pathlen] = '/';
        strncpy(&cake_list[cake_count].path[pathlen + 1], fn, sizeof(cake_list->path) - pathlen - 1);

        // Recurse into subdirectories
        if (fno.fattrib & AM_DIR) {
            // Using the path stored in the current cake.
            fr = load_cakes_info(cake_list[cake_count].path);
            if (fr != FR_OK) return fr;
            continue;
        }

        // Make sure the filename ends in .cake
        if (!memsearch(cake_list[cake_count].path, ".cake",
                sizeof(cake_list[cake_count].path), 6)) {
            continue;
        }

        // Open the file
        fr = f_open(&handle, cake_list[cake_count].path, FA_READ);
        if (fr != FR_OK) goto error;

        // Get the header
        unsigned int bytes_read = 0;
        struct cake_header header;
        fr = f_read(&handle, &header, sizeof(header), &bytes_read);
        if (fr != FR_OK) goto error;

        // Get the patch description
        const int desc_size = header.patches_offset - sizeof(header);
        fr = f_read(&handle, cake_list[cake_count].description, desc_size, &bytes_read);
        if (fr != FR_OK) goto error;

        fr = f_close(&handle);
        if (fr != FR_OK) goto error;

        cake_count++;
    }
    f_closedir(&dir);

    return 0;

error:
    f_close(&handle);
    f_closedir(&dir);
    return fr;
}
#endif
