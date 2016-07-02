#pragma once

// A file to keep track of all the file paths used throughout cakes.

// The "topdir"
#define PATH_CAKES "/cakes"

#define PATH_FIRMWARE_DIR PATH_CAKES "/firmware"

#define PATH_FIRMWARE PATH_FIRMWARE_DIR "/firmware.bin"
#define PATH_PATCHED_FIRMWARE PATH_FIRMWARE_DIR "/firmware_patched.bin"
#define PATH_FIRMKEY PATH_FIRMWARE_DIR "/firmkey.bin"
#define PATH_CETK PATH_FIRMWARE_DIR "/cetk"

#define PATH_TWL_FIRMWARE PATH_FIRMWARE_DIR "/twl_firmware.bin"
#define PATH_PATCHED_TWL_FIRMWARE PATH_FIRMWARE_DIR "/twl_firmware_patched.bin"
#define PATH_TWL_FIRMKEY PATH_FIRMWARE_DIR "/twl_firmkey.bin"
#define PATH_TWL_CETK PATH_FIRMWARE_DIR "/twl_cetk"

#define PATH_AGB_FIRMWARE PATH_FIRMWARE_DIR "/agb_firmware.bin"
#define PATH_PATCHED_AGB_FIRMWARE PATH_FIRMWARE_DIR "/agb_firmware_patched.bin"
#define PATH_AGB_FIRMKEY PATH_FIRMWARE_DIR "/agb_firmkey.bin"
#define PATH_AGB_CETK PATH_FIRMWARE_DIR "/agb_cetk"

#define PATH_MEMORY PATH_CAKES "/memory.bin"
#define PATH_UNSUPPORTED_FIRMWARE PATH_FIRMWARE_DIR "/firmware_unsupported.bin"
#define PATH_SLOT0X25KEYX "/slot0x25keyX.bin"
#define PATH_SLOT0X11KEY96 "/slot0x11key96.bin"
#define PATH_PATCHES PATH_CAKES "/patches"
#define PATH_CONFIG PATH_CAKES "/config.dat"
