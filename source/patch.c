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
#include "config.h"
#include "fatfs/ff.h"
#include "fatfs/sdmmc/sdmmc.h"
#include "external/crypto.h"
#else
#include <string.h>
#include <stdio.h>
#include "fcram.h"
#define print(string) puts(string)
#define draw_message(title, description) printf("-- %s:\n%s\n", title, description)
#endif

#define FORMAT_VERSION 1
#define MAX_MEMORY_PATCHES 0x10

enum types {
    TYPE_FIRM,
    TYPE_MEMORY,
    TYPE_USERLAND,
    TYPE_SYSMODULE
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

struct patch_version {
    union {
        struct {
            uint16_t firm_version;
            uint16_t console;
        } __attribute__((packed));
        uint32_t version;
    };
    uint32_t offset;
    uint32_t values_offset;
} __attribute__((packed));

struct memory_location {
    uint32_t location;
    uint32_t size;
    uint32_t used_size;
};

#ifndef STANDALONE
firm_h *firm_loc = (firm_h *)FCRAM_FIRM_LOC;
firm_h *twl_firm_loc = (firm_h *)FCRAM_TWL_FIRM_LOC;
firm_h *agb_firm_loc = (firm_h *)FCRAM_AGB_FIRM_LOC;

struct cake_info *cake_list = (struct cake_info *)FCRAM_CAKE_LIST;
unsigned int cake_count = 0;

static struct cake_header *firm_patch_temp = (struct cake_header *)FCRAM_FIRM_PATCH_TEMP;
#endif

uint32_t *memory_loc = (uint32_t *)FCRAM_MEMORY_LOC;
static void *current_memory_loc;

// Usable memory locations for arm9 memory patches.
struct memory_location memory_locations[] = {
    {
        .location = 0x01FF8000,
        .size = 0x00008000
    }, {.location = 0xFFFFFFFF}
};

// Allocates memory for usage AFTER booting.
void *allocate_memory(uint32_t *physical_address, size_t size)
{
    // Check for the remaining space in memory_loc
    if (current_memory_loc + size > (void *)memory_loc + FCRAM_SPACING) {
        print("Out of memory");
        draw_message("Out of memory", "We ran out of available memory to store memory patches.");
        return NULL;
    }

    struct memory_location *location;
    for (location = memory_locations; location->location != 0xFFFFFFFF; location++) {
        if (location->size - location->used_size > size) {
            // Calculate alignment to 4 bytes
            int align = 4 - size % 4;
            if (align == 4) align = 0;

            // Create the header
            struct memory_header *header = current_memory_loc;
            header->location = location->location + location->used_size;
            header->size = size + align;

            // Let everyone know we have allocated new memory
            current_memory_loc += sizeof(struct memory_header) + size + align;
            *memory_loc += sizeof(struct memory_header) + size + align;
            location->used_size += size + align;

            *physical_address = header->location;
            return header + 1;
        }
    }

    print("Out of system memory");
    draw_message("Out of system memory", "We ran out of usable space to install this memory patch to.");
    return NULL;
}

#ifndef STANDALONE
void *memsearch(void *start_pos, const void *search, const uint32_t size, const uint32_t size_search)
{
    // Searching backwards, since most of the stuff we'll search with this are near the end.
    for (void *pos = start_pos + size - size_search; pos >= start_pos; pos--) {
        if (memcmp(pos, search, size_search) == 0) {
            return pos;
        }
    }

    return NULL;
}

int get_emunand_offsets(const uint32_t location, uint32_t *offset, uint32_t *header)
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

int patch_options(void *address, const uint32_t size, const uint8_t options, const enum firm_types type)
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
            memcpy(pos, fcram_temp, AES_BLOCK_SIZE);
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
            print("Dunno where to set the offsets");
            draw_message("Dunno where to set the offsets",
                    "Hey, you! Yes, I'm looking at you.\n"
                    "Did you remember to have NAND and NCSD somewhere in the binary?\n"
                    "I believe not, because I couldn't find them.");
            return 1;
        }
    }
    if (options & patch_option_save && type == NATIVE_FIRM) {
        print("Patch option: Save firm");

        uint32_t *pos_native = memsearch(address, "NATF", size, 4);
        uint32_t *pos_twl = memsearch(address, "TWLF", size, 4);
        uint32_t *pos_agb = memsearch(address, "AGBF", size, 4);
        if (!pos_native && !pos_twl && !pos_agb) {
            print("Dunno where to set the offsets to the firm paths");
            draw_message("Dunno where to set the offsets to the firm paths",
                    "You should make sure you at least one of:\n"
                    "'NATF', 'TWLF' or 'AGBF' in your patch.");
            return 1;
        }

        save_firm = 1;

        uint32_t *pos[] = {pos_native, pos_twl, pos_agb};
        size_t size[] = {
            sizeof(PATH_PATCHED_FIRMWARE) * 2,
            sizeof(PATH_PATCHED_TWL_FIRMWARE) * 2,
            sizeof(PATH_PATCHED_AGB_FIRMWARE) * 2
        };
        wchar_t *string[] = {L"" PATH_PATCHED_FIRMWARE,
            L"" PATH_PATCHED_TWL_FIRMWARE, L"" PATH_PATCHED_AGB_FIRMWARE};

        // NOTE: This won't work unless all three arrays have the exact same amount of entries.

        for (unsigned int x = 0; x < sizeof(pos) / sizeof(uint32_t *); x++) {
            if (pos[x]) {
                uint32_t paddr = 0;
                char *addr = allocate_memory(&paddr, size[x] + 10);
                memcpy(addr, L"sdmc:", 10);
                memcpy(addr + 10, string[x], size[x]);

                *pos[x] = paddr;
            }
        }
    }

    return 0;
}
#else
int patch_options()
{
    return 0;
}
#endif

void patch_reset()
{
    // Reset the FIRM
    memcpy(firm_loc, firm_orig_loc, firm_size);
    if (current_twl_firm) memcpy(twl_firm_loc, twl_firm_orig_loc, twl_firm_size);
    if (current_agb_firm) memcpy(agb_firm_loc, agb_firm_orig_loc, agb_firm_size);

    // Reset memory
    *memory_loc = sizeof(*memory_loc);
    current_memory_loc = memory_loc + 1;
    for (struct memory_location *location = memory_locations;
            location->location != 0xFFFFFFFF; location++) {
        location->used_size = 0;
    }
}

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

    // This struct will be used to store and in the end patch all the locations of the memory patches.
    struct memory_ids {
        uint16_t id;
        uint32_t *patch_location;
        uint32_t memory_location;
    } memory_ids[MAX_MEMORY_PATCHES] = {0};

    struct patch *patches = (struct patch *)((uintptr_t)cake + cake->patches_offset);

    int applied = 0;

    for (struct patch *patch = patches;
            patch < patches + cake->patch_count; patch++) {
        void *patch_code = (void *)((uintptr_t)cake + patch->offset);
        struct patch_version *versions = (struct patch_version *)((uintptr_t)cake + patch->versions_offset);
        uint32_t *variables = (uint32_t *)((uintptr_t)cake + patch->variables_offset);
        struct patch_version *version = NULL;
        uint32_t *values;
        void *patch_location = NULL;

        // Process9 location cache for all the different firms
        static firm_section_h native_process9;
        static int native_process9_init;
        static firm_section_h twl_process9;
        static int twl_process9_init;
        static firm_section_h agb_process9;
        static int agb_process9_init;

        // Variables for the current firm
        firm_h *firm = NULL;
        struct firm_signature *firm_info = NULL;
        firm_section_h *process9;
        int *process9_init;

        // For firm and memory patches, we require some additional info.
        if (patch->type == TYPE_FIRM || patch->type == TYPE_MEMORY || patch->type == TYPE_SYSMODULE) {
            // Figure out which firm we have to patch
            switch (patch->firm_type) {
                case NATIVE_FIRM:
                    firm = firm_loc;
                    firm_info = current_firm;
                    process9 = &native_process9;
                    process9_init = &native_process9_init;
                    break;

                case TWL_FIRM:
                    firm = twl_firm_loc;
                    firm_info = current_twl_firm;
                    process9 = &twl_process9;
                    process9_init = &twl_process9_init;
                    break;

                case AGB_FIRM:
                    firm = agb_firm_loc;
                    firm_info = current_agb_firm;
                    process9 = &agb_process9;
                    process9_init = &agb_process9_init;
                    break;

                default:
                    print("Unsupported FIRM type");
                    draw_message("Unsupported FIRM type", "This cake uses an unknown or unsupported FIRM type.");
                    return 1;
            }

            if (!firm_info) {
                print("FIRM not loaded");
                draw_message("FIRM not loaded", "The FIRM this cake tries to patch isn't loaded.\nPlease make sure it's installed correctly and is loaded.");
                return 1;
            }

            // Look for the correct patch version info
            for (struct patch_version *patch_version = versions;
                    patch_version < versions + patch->version_count; patch_version++) {
                if (patch_version->console == firm_info->console &&
                        patch_version->firm_version == firm_info->version) {
                    version = patch_version;
                    break;
                }
            }

            if (!version) {
                // This specific patch doesn't support this FIRM version,
                //  but a different one might.
                continue;
            }

            // Apply all the variables for this version
            values = (uint32_t *)((uintptr_t)cake + version->values_offset);

            for (int x = 0; x < patch->variable_count; x++) {
                *(uint32_t *)(patch_code + variables[x]) = values[x];
            }
        }

        // Depending on the type, we have to use it in a different way
        if (patch->type == TYPE_FIRM) {
            // Look for Process9
            if (!*process9_init) {
                // Look for process9 in all sections
                for (firm_section_h *section = firm->section;
                        section < firm->section + 4; section++) {
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

            // Look for the location in the FIRM to apply the patch
            int x;
            for (x = 0; x < 5; x++) {
                firm_section_h *section;

                // Try process9 before anything else
                if (x == 0) {
                    section = process9;
                } else {
                    section = &firm->section[x - 1];

                    // Stop scanning at the end of the section list
                    if (section->address == 0) {
                        break;
                    }
                }

                if (version->offset >= section->address &&
                        version->offset < section->address + section->size) {
                    patch_location = (void *)((uintptr_t)firm + section->offset + (version->offset - section->address));

                    // Apply the patch
                    memcpy(patch_location, patch_code, patch->size);

                    // Apply whatever options it needs
                    if (patch->options) {
                        if (patch_options(patch_location, patch->size, patch->options, patch->type) != 0) {
                            return 1;
                        }
                    }

                    break;
                }
            }

            if (x >= 5) {
                print("Failed to apply patch");
                draw_message("Failed to apply patch", "The location where the patch should be applied could not be found");
                return 1;
            }

        } else if (patch->type == TYPE_MEMORY) {
            // Allocate memory for the patch
            void *memory = allocate_memory((uint32_t *)&patch_location, patch->size);
            if (!memory) return 1;

            // Copy the code
            memcpy(memory, patch_code, patch->size);

            // Apply whatever options it needs
            if (patch->options) {
                if (patch_options(memory, patch->size, patch->options, patch->type) != 0) {
                    return 1;
                }
            }

        } else if (patch->type == TYPE_SYSMODULE) {
            // Look for the section that holds all the sysmodules
            firm_section_h *sysmodule_section = NULL;
            for (firm_section_h *section = firm->section;
                    section < firm->section + 4; section++) {
                if (section->address == 0x1FF00000 && section->type == FIRM_TYPE_ARM11) {
                    sysmodule_section = section;
                    break;
                }
            }

            if (!sysmodule_section) {
                print("Couldn't find sysmodule section");
                draw_message("Couldn't find sysmodule section", "The patcher was unable to find the section where the sysmodules are stored in this firmware.");
                return 1;
            }

            ncch_h *module = patch_code;
            ncch_h *sysmodule = (ncch_h *)((uintptr_t)firm + sysmodule_section->offset);

            // Check if we want to replace an existing sysmodule
            while (sysmodule->magic == NCCH_MAGIC) {
                if (memcmp(sysmodule->programID, module->programID, 8) == 0) {
                    if (module->contentSize > sysmodule->contentSize) {
                        // TODO: Expanding a module's size isn't supported yet...
                        continue;
                    }

                    // Move the remaining modules closer
                    if (module->contentSize < sysmodule->contentSize) {
                        int remaining_size = sysmodule_section->size - (((uintptr_t)sysmodule + sysmodule->contentSize * 0x200) - ((uintptr_t)firm + sysmodule_section->offset));
                        memmove((char *)sysmodule + module->contentSize * 0x200, (char *)sysmodule + sysmodule->contentSize * 0x200, remaining_size);
                    }

                    // Copy the module into the firm
                    memcpy(sysmodule, module, module->contentSize * 0x200);

                    break;
                }

                sysmodule = (ncch_h *)((uintptr_t)sysmodule + sysmodule->contentSize * 0x200);
            }

            // TODO: Adding a new sysmodule is not supported yet...
            if (sysmodule->magic != NCCH_MAGIC) {
                print("Unuspported feature");
                draw_message("Unsupported feature", "This CakesFW version doesn't support injecting bigger sysmodules than those available or adding new ones yet.");
                return 1;
            }

        } else {
            print("Unsupported patch type");
            draw_message("Unsupported patch type", "This cake uses an unknown or unsupported patch type.");
            return 1;
        }

        // For firm and memory patches, add some info to the memory_ids array.
        if ((patch->type == TYPE_FIRM && patch->memory_id) || patch->type == TYPE_MEMORY) {
            struct memory_ids *copy_id = NULL;
            int found = 0;
            struct memory_ids *memory_id;
            // Look for the correct memory_id
            for (memory_id = memory_ids;
                    memory_id < memory_ids + MAX_MEMORY_PATCHES && memory_id->id;
                    memory_id++) {
                if (memory_id->id == patch->memory_id) {
                    // We found the entry.
                    found++;

                    if (patch->type == TYPE_FIRM) {
                        // If this hook has already been set, copy the current one memory_id and look for more.
                        if (memory_id->patch_location) {
                            copy_id = memory_id;
                            continue;
                        }
                        memory_id->patch_location = patch_location + patch->memory_offset;
                        break;
                    } else {
                        memory_id->memory_location = (uintptr_t)patch_location;
                    }
                }
            }

            if (memory_id >= memory_ids + MAX_MEMORY_PATCHES) {
                // If we stopped looping because there's no more room, end.
                print("Too many memory patches");
                draw_message("Too many memory patches", "This cake contains too many different memory ids. We can't hold them all.");
                return 1;
            } else if (!memory_id->id && (!found || copy_id)) {
                // Else, create a new entry.
                memory_id->id = patch->memory_id;
                // Fill the data we need.
                if (patch->type == TYPE_FIRM) {
                    if (copy_id) {
                        memory_id->memory_location = copy_id->memory_location;
                    }
                    memory_id->patch_location = patch_location + patch->memory_offset;
                } else {
                    memory_id->memory_location = (uintptr_t)patch_location;
                }
            }
        }

        applied = 1;
    }

    if (!applied) {
        print("Unable to apply cake");
        draw_message("Unable to apply cake",
                "This cake was unable to be applied correctly,\n"
                "  probably because there was no patch available\n"
                "  for your current FIRM version");
        return 1;
    }

    // Make all the hook patches point to the right memory location.
    for (struct memory_ids *memory_id = memory_ids;
            memory_id < memory_ids + MAX_MEMORY_PATCHES && memory_id->id;
            memory_id++) {
        if (!memory_id->patch_location || !memory_id->memory_location) {
            print("Missing hook or memory patch");
            draw_message("Missing hook or memory patch", "This cake lacks either a memory patch or a hook.");
            return 1;
        }

        *memory_id->patch_location = memory_id->memory_location;
    }

    return 0;
}

#ifndef STANDALONE
int patch_firm_all()
{
    print("Resetting FIRM...");
    patch_reset();

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
    cake_count = 0;
    memset(cake_selected, 0, MAX_CAKES);

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

        // Build the path string
        memcpy(cake_list[cake_count].path, dirpath, pathlen);
        cake_list[cake_count].path[pathlen] = '/';
        strncpy(&cake_list[cake_count].path[pathlen + 1], fno.fname, sizeof(cake_list->path) - pathlen - 1);
        cake_list[cake_count].path[sizeof(cake_list->path) - 1] = 0;  // Make sure it terminates.

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
        if (fr != FR_OK || bytes_read != sizeof(header)) goto error;

        // Get patch list
        bytes_read = 0;
        struct patch patches[header.patch_count];
        fr = f_lseek(&handle, header.patches_offset);
        if (fr != FR_OK) goto error;
        fr = f_read(&handle, patches, sizeof(patches), &bytes_read);
        if (fr != FR_OK || bytes_read != sizeof(patches)) goto error;

        // Check if the right FIRM version is available.
        for (struct patch *patch = patches;
                patch < patches + header.patch_count; patch++) {
            // We check if the cake file has at least one patch that can be applied.
            // If it's a patch which needs a FIRM, we check if the correct FIRM version is available.
            if (patch->type == TYPE_FIRM || patch->type == TYPE_MEMORY || patch->type == TYPE_SYSMODULE) {
                // Read the versions.
                bytes_read = 0;
                struct patch_version versions[patch->version_count];
                fr = f_lseek(&handle, patch->versions_offset);
                if (fr != FR_OK) goto error;
                fr = f_read(&handle, versions, sizeof(versions), &bytes_read);
                if (fr != FR_OK || bytes_read != sizeof(versions)) goto error;

                struct firm_signature *firm_info = NULL;

                switch (patch->firm_type) {
                    case NATIVE_FIRM:
                        firm_info = current_firm;
                        break;
                    case TWL_FIRM:
                        firm_info = current_twl_firm;
                        break;
                    case AGB_FIRM:
                        firm_info = current_agb_firm;
                        break;
                }

                // If the FIRM isn't loaded, this patch can't be applied.
                if (!firm_info) continue;

                // If this patch doesn't have a version for the currently loaded FIRM, it can't be applied.
                for (struct patch_version *version = versions;
                        version < versions + patch->version_count; version++) {
                    if (version->console == firm_info->console &&
                            version->firm_version == firm_info->version) {
                        goto patch_applicable;
                    }
                }
            }
        }

        // This patch can't be applied.
        continue;
patch_applicable:;

        // Get the patch description
        const int desc_size = header.patches_offset - sizeof(header);
        fr = f_lseek(&handle, 3);
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
