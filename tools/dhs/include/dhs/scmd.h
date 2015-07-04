#ifndef __SCMD_H_
#define __SCMD_H_

#include <stdint.h>

#define SCMD_MAGIC 0x43525443

enum SCMD
{
	SCMD_INFO = 1,
	SCMD_DUMP = 2,
	SCMD_PATCH = 3,
	SCMD_INSTALL = 4,
	SCMD_DELETE = 5,
	SCMD_INSTALLFIRM = 6,
	SCMD_TRANSLATE = 7
};

enum SCMD_MEMTYPE
{
	MEMTYPE_KERNEL = 0,
	MEMTYPE_PROCESS,
	MEMTYPE_PHYSICAL
};

typedef struct KMemInfo
{
	void* addr;
	uint32_t total_pages;
	uint32_t binfo_count;		// KBlockInfo count for section
	void* binfo_first;			// Pointer to KLinkedListNode that holds a pointer to the first KBlockInfo object for that section
	void* binfo_last;			// Pointer to KLinkedListNode that holds a pointer to the last KBlockInfo object for that section
} KMemInfo;

typedef struct KCodeSet
{
	void* vtable;
	uint32_t ref_count;
	KMemInfo text;
	KMemInfo rodata;
	KMemInfo data;
	uint32_t text_pages;
	uint32_t ro_pages;
	uint32_t rw_pages;
	uint32_t namehi;
	uint32_t namelo;
	uint32_t unk;
	uint64_t titleid;
} KCodeSet;

typedef struct KProcess_4
{
	uint8_t pad0[0x54];
	uint32_t mmu_table;		// 0x54
	uint8_t pad1[0x50];
	KCodeSet* kcodeset;		// 0xA8
	uint32_t pid;			// 0xAC
} KProcess_4;

typedef struct KProcess_8
{
	uint8_t pad0[0x5C];
	uint32_t mmu_table;		// 0x5C
	uint8_t pad1[0x50];
	KCodeSet* kcodeset;		// 0xB0
	uint32_t pid;			// 0xB4
} KProcess_8;

typedef struct scmdreq_s
{
	uint32_t magic;
	uint32_t cmd;
} scmdreq_s;

typedef struct scmdack_s
{
	uint32_t magic;
} scmdack_s;

typedef struct scmdres_info_s
{
	uint32_t res;
	uint32_t kernelver;
	uint32_t firmver;
	uint32_t kernel_syscorever;
	uint32_t firm_syscorever;
	uint32_t unit_info;
	uint32_t kernel_ctrsdkver;
	uint32_t firm_ctrsdkver;
	uint32_t app_memtype;
	uint32_t app_memalloc;
	uint32_t sys_memalloc;
	uint32_t base_memalloc;
	uint32_t heap;
	uint32_t linheap;
	uint32_t pid;
	uint32_t kprocess_addr;
} scmdres_info_s;

typedef struct scmdreq_dump_s
{
	scmdreq_s req;
	void* addr;
	uint32_t size;
	uint32_t is_pa;
} scmdreq_dump_s;

typedef struct scmdres_dump_s
{
	uint32_t res;
	uint32_t size;
} scmdres_dump_s;

typedef struct scmdreq_patch_s
{
	scmdreq_s req;
	void* addr;
	uint32_t size;
	uint32_t is_pa;
} scmdreq_patch_s;

typedef struct scmdres_patch_s
{
	uint32_t res;
	uint32_t size;
} scmdres_patch_s;

typedef struct scmdreq_install_s
{
	scmdreq_s req;
	uint32_t media;
	uint32_t filesize;
} scmdreq_install_s;

typedef struct scmdres_install_s
{
	uint32_t res;
} scmdres_install_s;

typedef struct scmdreq_delete_s
{
	scmdreq_s req;
	uint32_t media;
	uint64_t titleid;
} scmdreq_delete_s;

typedef struct scmdres_delete_s
{
	uint32_t res;
} scmdres_delete_s;

typedef struct scmdres_installfirm_s
{
	uint32_t res;
} scmdres_installfirm_s;

typedef struct scmdreq_translate_s
{
	scmdreq_s req;
	uint32_t from;
	uint32_t to;
	uint32_t namehi;
	uint32_t namelo;
	uint32_t address;
} scmdreq_translate_s;

typedef struct scmdres_translate_s
{
	uint32_t res;
	uint32_t address;
} scmdres_translate_s;

#endif /*__SCMD_H_*/
