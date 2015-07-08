#include <string.h>
#include "dhs_patch/dhs_patch_compat.h"
#include "dhs_patch.h"
#include "svc.h"

#define REG_HID					((volatile uint32_t*)0x10146000)
#define HID_RT					((uint32_t)0x100)
#define HID_LT					((uint32_t)0x200)

extern dhs_a9_compat_s a9compat;
extern dhs_a11_compat_s a11compat;

void doExploit();
void dataabort_hook();
void ld11_hook();

extern uint32_t dump_only;

__attribute__((naked))
int main(void)
{
	doExploit();

	asm
	(
		"ldr r0, =hook_load\n\t"
		"ldr r0, [r0]\n\t"
		"ldr r1, =hook_return\n\t"
		"ldr pc, [r1]\n\t"
	);
}

__attribute__((naked))
void mpuSetup(void)
{
	asm
	(
		"mov r0, #0\n\t"
		"mcr p15, 0, r0, c6, c4, 0\n\t"	// Disable region 4
		"ldr r0, =0x10000037\n\t"		// 10000000 256MB region 3
		"mcr p15, 0, r0, c6, c3, 0\n\t"
		"mrc p15, 0, r0, c6, c6, 0\n\t"
		"bic r0, r0, #1\n\t"
		"mcr p15, 0, r0, c6, c6, 0\n\t" // Disable region 6
		"mrc p15, 0, r0, c6, c7, 0\n\t"
		"bic r0, r0, #1\n\t"
		"mcr p15, 0, r0, c6, c7, 0\n\t" // Disable region 7
		"bx lr\n\t"
	);
}

__attribute__((naked))
void mpuSetupWrap(void)
{
	asm
	(
		"mov r0, sp\n\t"
		"stmfd sp!, {r0, r5, lr}\n\t"
		"mrs r5, cpsr\n\t"
		"orr r2, r5, #0x80\n\t"		// Disable interrupts
		"msr cpsr_c, r2\n\t"		// Update the control bits in the CPSR
		"ldr r4, =mpuSetup\n\t"
		"blx r4\n\t"
		"msr cpsr_c, r5\n\t"		// Restore original interrupt settings
		"ldmfd sp!, {r0, r5, lr}\n\t"
		"mov sp, r0\n\t"
		"bx lr\n\t"
	);
}

uint32_t getBranchInst(int32_t from, int32_t to, int link)
{
	from += 8;
	to -= from;
	to = to / 4;

	uint32_t b1 = to & 0x20000000u;
	to &= ~0xFF800000;
	if(b1)
	{
		to |= 0x800000;
	}

	if(link == 0) // branch
		to |= 0xEA000000;
	else // branch & link
		to |= 0xEB000000;

	return to;
}

extern uint8_t _a11_hook_start;
extern uint8_t _a11_hook_end;

void patchARM11Kernel(void)
{
	a11compat.buffer = a9compat.buffer_va;
	a11compat.translate_va = a9compat.translate_va;
	a11compat.mmu_table_offset = a9compat.mmu_table_offset;
	a11compat.exit_process = a9compat.exit_process;

	memset((void*)a9compat.buffer_pa, 0, 0xC00);
	memcpy((void*)a9compat.hooks_pa, &_a11_hook_start, &_a11_hook_end - &_a11_hook_start);

	uint32_t a11_base_pa = a9compat.kernel_base_pa;
	uint32_t a11_base_va = a9compat.kernel_base_va;

	uint32_t offset = a9compat.data_abort_offset;
	*(uint32_t*)(a11_base_pa + offset + 8) = a9compat.hooks_va + ((uint8_t*)dataabort_hook - &_a11_hook_start);

	offset = a9compat.ld11_offset;
	*(uint32_t*)(a11_base_pa + offset) = getBranchInst(a11_base_va + offset, a9compat.hooks_va + ((uint8_t*)ld11_hook - &_a11_hook_start), 1);
}

void dumpAXIWRAM(void)
{
	dump_to_file("sdmc:/axiwram.bin", (void*)0x1FF80000, 0x80000);
}

void readARM11CodeBin(uint32_t* text, uint32_t size)
{
	// SVC Handler patch, allow all svcs
	*a9compat.svc_patch_pa = 0;

	dump_to_mem("sdmc:/dhs.bin", text, 0);
}

void processA11Hooks(void)
{
	ld_data_s* data = (ld_data_s*) (a9compat.buffer_pa + 0x200);
	if(data->magic == HAXX)
	{
		if(data->type == LD11)
		{
			if(data->name_lo == PROC_NAME_LO && data->name_hi == PROC_NAME_HI) // dlp
			{
				readARM11CodeBin((uint32_t*)data->code_addr, data->total_pages << 0xC);
				//dump_to_file("sdmc:/buf.bin", data, 0x200);
			}
		}
		else if(data->type == EDBG)
		{
			dump_to_file("sdmc:/deb.bin", data, 0x200);
		}

		memset(data, 0, data->size);
	}
}

void bgThreadEntry(void)
{
	if(!dump_only)
	{
		while(1)
		{
			processA11Hooks();
			if((*REG_HID & HID_RT) == 0)
			{
				dumpAXIWRAM();
				while(1);
			}
		}
	}
	else
	{
		svcSleepThread(0x540BE400, 2); // Sleep for 10 seconds
		dumpAXIWRAM();

		while(1);
	}
}

void doExploit(void)
{
	svcBackdoor(mpuSetupWrap);
	if(!dump_only)
		patchARM11Kernel();

	Handle thread;
	void* arg = NULL;
	void* stackTop = (void*)0x1FFAFB4;
	svcCreateThread(&thread, bgThreadEntry, arg, stackTop, 0x3F, -2);
}
