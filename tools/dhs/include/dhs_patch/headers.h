#ifndef __HEADERS_H
#define __HEADERS_H

// http://3dbrew.org/wiki/NCCH/Extended_Header
typedef struct code_set_info
{
	uint32_t address;
	uint32_t phy_region_size;	// Physical region size (in page-multiples)
	uint32_t size;				// size in bytes
} code_set_info; // 0x0C

typedef struct system_info
{
	uint32_t saveDataSize[2];
	uint32_t jumpID[2];
	uint8_t reserved[0x30];
} system_info; // 0x40

typedef struct system_control_info
{
	uint8_t appTitle[8];
	char reserved1[5];
	char flag;
	char remasterVersion[2];
	code_set_info textCodeSet;
	uint32_t stackSize;
	code_set_info roCodeSet;
	char reserved2[4];
	code_set_info dataCodeSet;
	uint32_t bssSize;
	char depends[0x180]; // Dependency Module (Program ID) List 48*8
	system_info systemInfo;
} system_control_info; // 0x200

/*
Storage Info
Offset	Size	Description
0x0	0x8	Extdata ID
0x8	0x8	System Save Data Ids
0x10	0x8	Storage Accessable Unique Ids
0x18	0x7	File System Access Info
0x1F	0x1	Other Attributes
 */

typedef struct arm11_storage_info
{
	uint8_t extdataID[8]; // Extdata ID
	uint8_t savedataIDs[8]; // System Save Data Ids
	uint8_t saui[8]; // Storage Accessable Unique Ids
	uint8_t fsai[7]; // File System Access Info
	uint8_t other;
} arm11_storage_info;

typedef struct arm11_local_system_cap
{
	uint8_t programID[8];
	uint32_t core_version;
	uint8_t flag1;
	uint8_t flag2;
	uint8_t flag0;
	uint8_t priority;
	uint32_t limits[8];
	arm11_storage_info storage_info;
	uint8_t service_access_control[0x20 * 8];
	uint8_t ex_service_access_control[0x02 * 8];
	uint8_t pad0[0xF];
	uint8_t resource_limit_cat; // Resource Limit Category. (0 = APPLICATION, 1 = SYS_APPLET, 2 = LIB_APPLET, 3 = OTHER(sysmodules running under the BASE memregion))
} arm11_local_system_cap;

typedef struct arm11_kernel_cap
{
	uint32_t descriptors[28];
	uint32_t reserved[4];
} arm11_kernel_cap;

typedef struct arm9_access_control
{
	uint8_t descriptors[0xF];
	uint8_t version;
} arm9_access_control;

typedef struct access_control_info
{
	arm11_local_system_cap alsc;
	arm11_kernel_cap akc;
	arm9_access_control aac;
} access_control_info;

typedef struct ncch_ex_h
{
	system_control_info sci;
	access_control_info aci;
	uint8_t accessDescSig[0x100];
	uint8_t ncchPubKey[0x100];
	uint8_t aciLim[0x200];
} ncch_ex_h; // 0x800

#endif
