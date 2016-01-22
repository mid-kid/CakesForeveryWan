#include "patch.h"

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "draw.h"
#include "memfuncs.h"
#include "fs.h"
#include "menu.h"
#include "firm.h"
#include "fcram.h"
#include "paths.h"
#include "crypto.h"
#include "config.h"
#include "fatfs/ff.h"
#include "fatfs/sdmmc/sdmmc.h"

enum types {
    NATIVE_FIRM,
    TWL_FIRM,
    AGB_FIRM
};

struct cake_header {
    uint8_t count: 8;
    uint8_t firm_ver: 8;
    enum types type: 4;
    enum consoles console: 4;
    uint8_t patches_offset: 8;
    char name[];
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

int patch_options(void *address, uint32_t size, uint8_t options, enum types type)
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

int patch_firm(char *filename)
{
    if (read_file(firm_patch_temp, filename, FCRAM_SPACING) != 0) {
        print("Failed to load patch");
        draw_message("Failed to load patch", "Please make sure all the patches you want\n  to apply actually exist on the SD card.");
        return 1;
    }
    print("Applying patch:");
    print(firm_patch_temp->name);

    firm_h *firm = NULL;

    switch (firm_patch_temp->type) {
        case NATIVE_FIRM:
            firm = firm_loc;
            break;
        case AGB_FIRM:
            firm = agb_firm_loc;
            break;
        default:
            print("Unsupported patch type");
            draw_message("Unsupported patch type", "This patch is of an unsupported type.\nPlease make sure it's supported if you wish to use it");
            return 1;
    }

    struct patch *patches = (void *)firm_patch_temp + firm_patch_temp->patches_offset;

    firm_section_h *sections = firm->section;
    firm_section_h process9;
    memset(&process9, 0, sizeof(firm_section_h));

    // Assuming Process9 is in section 2
    for (int x = 0; x < 4; x++) {
        if (sections[x].address == 0) {
            break;
        } else if (sections[x].type != FIRM_TYPE_ARM9) {
            continue;
        }

        uint32_t *arm9section = (uint32_t *)((void *)firm + sections[x].offset);
        for (uint32_t i = 0; i < (sections[x].size / sizeof(uint32_t)); i += 2) {
            // 'Process9' <- this will be in exheader
            if (arm9section[i] == 0x636F7250 && arm9section[i + 1] == 0x39737365) {
                ncch_h *ncch = (ncch_h *)((uint8_t *)arm9section + (i * 4)
                        - sizeof(ncch_h));
                if (ncch->magic == NCCH_MAGIC) {
                    ncch_ex_h *p9exheader = (ncch_ex_h *)&ncch[1];
                    exefs_h* p9exefs = (exefs_h *)&p9exheader[1];
                    process9.address = (void *)p9exheader->sci.textCodeSet.address;
                    process9.size = p9exefs->fileHeaders[0].size;
                    process9.offset = ((uint32_t)ncch - (uint32_t)firm)
                            + sizeof(ncch_h) + sizeof(ncch_ex_h) + sizeof(exefs_h);
                    goto found_process9;
                }
            }
        }
    }
    // Can't find process9
    return 1;
found_process9:

    for (uint8_t i = 0; i < firm_patch_temp->count; i++) {
        if (patches[i].address >= process9.address &&
              patches[i].address < process9.address + process9.size) {
            void *offset = (void *)firm + (uintptr_t)(process9.offset + (patches[i].address - process9.address));
            memcpy(offset, (uintptr_t)firm_patch_temp + patches[i].offset, patches[i].size);

            if (patches[i].options) {
                if (patch_options(offset, patches[i].size, patches[i].options, firm_patch_temp->type) != 0) {
                    return 1;
                }
            }

            continue;
        }

        for (int x = 0; x < 4; x++) {
            if (sections[x].address == 0) {
                break;
            }

            if (patches[i].address >= sections[x].address &&
                  patches[i].address < sections[x].address + sections[x].size) {
                void *offset = (void *)firm + (uintptr_t)(sections[x].offset + (patches[i].address - sections[x].address));
                memcpy(offset, (uintptr_t)firm_patch_temp + patches[i].offset, patches[i].size);

                if (patches[i].options) {
                    if (patch_options(offset, patches[i].size, patches[i].options, firm_patch_temp->type) != 0) {
                        return 1;
                    }
                }

                break;
            }
        }
    }

    return 0;
}

int patch_firm_all()
{
    for (unsigned int i = 0; i < cake_count; i++) {
        if (cake_selected[i]) {
            if (patch_firm(cake_list[i].path)) return 1;
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

        // Only add patches applicable to the loaded firms
        struct firm_signature *current = NULL;
        switch (header.type) {
            case NATIVE_FIRM:
                current = current_firm;
                break;
            case AGB_FIRM:
                current = current_agb_firm;
                break;
            default:
                continue;
        }
        if (!current ||
                header.firm_ver != current->version ||
                header.console != current->console) {
            continue;
        }

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
