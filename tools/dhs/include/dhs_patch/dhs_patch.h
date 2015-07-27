#ifndef __DHS_PATCH_H_
#define __DHS_PATCH_H_

#define HAXX					0x58584148
#define LD11					0x3131444C
#define EDBG					0x58584148

#define PROC_NAME_LO			0x706C64
#define PROC_NAME_HI			0

typedef struct abort_data_s
{
	uint32_t magic;
	uint32_t type;
	uint32_t size;

	uint32_t regs[21];

	uint32_t name_lo;
	uint32_t name_hi;
	void* mmu_table;

	uint32_t dfault;
	uint32_t ifault;
	void* dfault_addr;
} abort_data_s;

typedef struct ld_data_s
{
	uint32_t magic;
	uint32_t type;
	uint32_t size;

	uint32_t total_pages;
	void* code_addr;
	uint32_t name_lo;
	uint32_t name_hi;
} ld_data_s;

typedef struct ssr_data_s
{
	uint32_t magic;
	uint32_t name_lo;
	uint32_t name_hi;
	uint32_t processed;
	uint32_t handle;
	uint32_t cmd_buffer[];
} ssr_data_s;

typedef struct proc_data proc_data;

#endif /*__DHS_PATCH_H_*/
