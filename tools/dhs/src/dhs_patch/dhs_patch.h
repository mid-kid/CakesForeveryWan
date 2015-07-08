#ifndef DHS_PATCH_H
#define DHS_PATCH_H

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

typedef struct proc_section
{
	void* addr;
	uint32_t pages;
} proc_section;

typedef struct proc_data
{
	uint32_t name_lo;
	uint32_t name_hi;
	uint32_t unk[2];
	proc_section text;
	proc_section ro;
	proc_section data;
	uint32_t x_pages;
	uint32_t ro_pages;
	uint32_t rw_pages;
} proc_data;

#endif /*DHS_PATCH_H*/
