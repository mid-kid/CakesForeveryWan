#include "dhs_patch/dhs_patch_compat.h"
#include "dhs_patch/dhs_patch.h"
#include "dhs/kobj.h"

#include <stdint.h>

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

// Dirty trick to get PIC, I'm going to regret this
dhs_a11_compat_s* get_compat();

typedef uint32_t Handle;

__attribute__((section(".text.a11")))
void waitArm9(volatile uint32_t* buffer)
{
	do
	{
		asm volatile
		(
			"mov r2, #0\n\t"
			"mcr p15, 0, r2, c7, c14, 0\n\t"	// Clean and Invalidate Entire Data Cache
			:::"r2"
		);
	} while(buffer[0] == HAXX);
}

__attribute__((section(".text.a11")))
void abortHookHandler(uint32_t* regs)
{
	dhs_a11_compat_s* compat = get_compat();

	abort_data_s* data = (abort_data_s*) (compat->buffer + 0x200);
	waitArm9((uint32_t*)data);

	data->type = EDBG;
	data->size = sizeof(abort_data_s);

	for(int i = 0; i < 21; i++)
		data->regs[i] = regs[i];

	void* mmu_table = 0;
	uint32_t name_lo = 0;
	uint32_t name_hi = 0;
	uint8_t* kprocess = *(uint8_t**) 0xFFFF9004;
	if(kprocess != NULL)
	{
		mmu_table = (void*)(kprocess + compat->mmu_table_offset);

		KCodeSet* codeset = (KCodeSet*)*(uint32_t*)(kprocess + compat->codeset_offset);
		if(codeset != NULL)
		{
			name_lo = codeset->namelo;
			name_hi = codeset->namehi;
		}
	}

	data->mmu_table = mmu_table;
	data->name_lo = name_lo;
	data->name_hi = name_hi;

	uint32_t dfault, ifault;
	void* dfault_addr;
	asm volatile
	(
		"mrc p15, 0, %0, c5, c0, 0\n\t"		// Read Data Fault Status Register
		"mrc p15, 0, %1, c5, c0, 1\n\t"		// Read Instruction Fault Status Register
		"mrc p15, 0, %2, c6, c0, 0\n\t"		// Read Data Fault Address Register
		:"=r"(dfault), "=r"(ifault), "=r"(dfault_addr)
	);
	data->dfault = dfault;
	data->ifault = ifault;
	data->dfault_addr = dfault_addr;

	data->magic = HAXX;
	asm volatile
	(
		"mov r0, #0\n\t"
		"mcr p15, 0, r0, c7, c10, 5\n\t"		// Data Memory Barrier
		"mcr p15, 0, r0, c7, c14, 0\n\t"		// Clean and Invalidate Entire Data Cache
		:::"r0"
	);

	while(1);
}

__attribute__((section(".text.a11")))
void ldHookHandler(proc_data* procData, void* addr_va)
{
	dhs_a11_compat_s* compat = get_compat();

	ld_data_s* data = (ld_data_s*) (compat->buffer + 0x200);
	waitArm9((uint32_t*)data);

	data->type = LD11;
	data->size = sizeof(ld_data_s);

	data->name_lo = procData->name_lo;
	data->name_hi = procData->name_hi;
	data->total_pages = procData->x_pages + procData->ro_pages + procData->rw_pages;

	uint8_t* kprocess = *(uint8_t**) 0xFFFF9004;
	void* mmu_table = (void*)(kprocess + compat->mmu_table_offset);
	data->code_addr = compat->translate_va(mmu_table, addr_va);

	data->magic = HAXX;
	asm volatile
	(
		"mov r0, #0\n\t"
		"mcr p15, 0, r0, c7, c10, 5\n\t"		// Data Memory Barrier
		"mcr p15, 0, r0, c7, c14, 0\n\t"		// Clean and Invalidate Entire Data Cache
		:::"r0"
	);
	waitArm9((uint32_t*)data);
}

__attribute__((section(".text.a11")))
void ssrHookHandler(Handle handle)
{
	dhs_a11_compat_s* compat = get_compat();

	volatile ssr_data_s* data = (volatile ssr_data_s*) compat->buffer;
	if(data->magic == HAXX)
	{
		uint8_t* kprocess = *(uint8_t**) 0xFFFF9004;
		KCodeSet* codeset = (KCodeSet*)*(uint32_t*)(kprocess + compat->codeset_offset);
		if(codeset)
		{
			if(codeset->namelo == data->name_lo && codeset->namehi == data->name_hi)
			{
				uint32_t* tls;
				asm volatile
				(
					"mrc p15, 0, %0, c13, c0, 3"
					:"=r"(tls)
				);
				uint32_t* cmd_buffer = tls + 0x20;

				data->handle = handle;
				for(int i = 0; i < 0x100 / sizeof(uint32_t); i++)
					data->cmd_buffer[i] = cmd_buffer[i];

				data->processed = 0;
				while(!data->processed);
			}
		}
	}
}

__attribute__((section(".text.a11")))
void svcDevHandler(void(*fun)())
{
	fun();
}
