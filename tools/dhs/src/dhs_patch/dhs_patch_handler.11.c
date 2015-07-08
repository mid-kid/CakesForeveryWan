#include "dhs_patch/dhs_patch_compat.h"
#include "dhs_patch.h"
#include <stdint.h>

// Dirty trick to get PIC, I'm going to regret this
dhs_a11_compat_s* get_compat();

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

		uint32_t* codeset = (uint32_t*)(kprocess + compat->codeset_offset);
		if(codeset != NULL)
		{
			name_lo = data->name_lo;
			name_hi = data->name_hi;
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
