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
	SCMD_INSTALLFIRM = 6
};

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

#endif /*__SCMD_H_*/
