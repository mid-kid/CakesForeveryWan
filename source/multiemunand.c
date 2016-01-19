#include "menu.h"
#include "draw.h"
#include "multiemunand.h"
#include "config.h"

uint32_t emunand_offset(uint32_t nand_size)
{
    uint32_t offset = 0;
    uint32_t emuoffset = 0;

    if (nand_size > 0x200000) { //If nand_size is more than 1GiB (0x40000000 divided by 0x200 (512 KiB, SD card sector size)),
    	offset = 0x400000;        //setting nand_offset to 2GiB (0x80000000 divided by 0x200 (512 KiB, SD card sector size))
    } else offset = 0x200000;
    
    emuoffset = (config->emunand_selected*offset); //Calculating SD offset of selected EmuNAND
    return emuoffset; 
}

void menu_emunand_config()
{
    char *confirm[] =  {"No",
                        "Yes"};

    char *options[] = {"First EmuNAND",
                       "Second EmuNAND",
                       "Third EmuNAND",
                       "Fourth EmuNAND"};
    int result = draw_menu("Multi EmuNAND configuration menu", 0, sizeof(options) / sizeof(char *), options);
    config->emunand_selected = result;

    if (!(config->autoboot_enabled && (draw_menu("Use selected EmuNAND for autobooting?", 0, sizeof(confirm) / sizeof(char *), confirm) == 0))) {
      patches_modified = 1;
    }
    emunand_status();
}

void emunand_status()
{	
    char *emunandstring[] = {"\nMulti EmuNAND Status: First EmuNAND",
                             "\nMulti EmuNAND Status: Second EmuNAND",
                             "\nMulti EmuNAND Status: Third EmuNAND",
                             "\nMulti EmuNAND Status: Fourth EmuNAND"};
    print(emunandstring[config->emunand_selected]);
}
