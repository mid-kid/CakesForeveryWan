#ifndef __SCMD_H_
#define __SCMD_H_

#include <stdint.h>

#define SCMD_MAGIC 0x43525443

enum SCMD
{
	SCMD_INFO = 1,
	SCMD_DUMP,
	SCMD_PATCH ,
	SCMD_INSTALL,
	SCMD_DELETE,
	SCMD_INSTALLFIRM,
	SCMD_TRANSLATE,
	SCMD_GETPROCESS_LIST,
	SCMD_GETKPROCESS,
	SCMD_GETHANDLE,
	SCMD_SERVICE,
	SCMD_SERVICEMON,
};

enum SCMD_MEMTYPE
{
	MEMTYPE_KERNEL = 0,
	MEMTYPE_PROCESS,
	MEMTYPE_PHYSICAL
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

typedef struct scmdreq_translate_s
{
	scmdreq_s req;
	uint32_t from;
	uint32_t to;
	uint32_t namelo;
	uint32_t namehi;
	uint32_t address;
} scmdreq_translate_s;

typedef struct scmdres_translate_s
{
	uint32_t res;
	uint32_t address;
} scmdres_translate_s;

typedef struct scmdres_getprocess_list_s
{
	uint32_t res;
	uint32_t count;
	char names[0x100][8];
} scmdres_getprocess_list_s;

typedef struct scmdreq_getkprocess_s
{
	scmdreq_s req;
	uint32_t namelo;
	uint32_t namehi;
} scmdreq_getkprocess_s;

typedef struct scmdres_getkprocess_s
{
	uint32_t res;
	uint32_t kprocess;
} scmdres_getkprocess_s;

typedef struct scmdreq_gethandle_s
{
	scmdreq_s req;
	char name[8];
} scmdreq_gethandle_s;

typedef struct scmdres_gethandle_s
{
	uint32_t res;
	uint32_t handle;
} scmdres_gethandle_s;

typedef struct scmdreq_service_s
{
	scmdreq_s req;
	uint32_t handle;
	uint32_t header_code;
	uint32_t output_size;
	uint32_t argc;
	uint32_t argv[0x20]; // Variable length is nicer though..
} scmdreq_service_s;

typedef struct scmdres_service_s
{
	uint32_t res;
	uint32_t size;
} scmdres_service_s;

typedef struct scmdreq_servicemon_s
{
	scmdreq_s req;
	char name[8];
} scmdreq_servicemon_s;

typedef struct scmdres_servicemon_s
{
	uint32_t res;
	uint32_t handle;
	uint32_t outhandle;
	char name[8];
	uint32_t cmd_buffer[0x100 / sizeof(uint32_t)]; // 0x40
} scmdres_servicemon_s;

#endif /*__SCMD_H_*/
