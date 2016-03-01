#include <string.h>
#include "dhs_patch/dhs_patch_compat.h"
#include "dhs_patch/dhs_patch.h"
#include "dhs_patch/headers.h"
#include "svc.h"
#include "file.h"

#define REG_HID					((volatile uint32_t*)0x10146000)
#define HID_RT					((uint32_t)0x100)
#define HID_LT					((uint32_t)0x200)

extern dhs_a9_compat_s a9compat;
extern dhs_a11_compat_s a11compat;

void doExploit();
void dataabort_hook();
void ld11_hook();
void ssr_hook();
void svcDevHandler(void(*fun)());

extern uint32_t dump_only;

__attribute__((naked))
int main()
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
void mpuSetup()
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
void mpuSetupWrap()
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

void patchExHeader(ncch_ex_h* exheader)
{
	if(*(uint32_t*)&exheader->sci.appTitle[0] == PROC_NAME_LO && *(uint32_t*)&exheader->sci.appTitle[4] == PROC_NAME_HI)
	{
		// File System Access Info, sdmc:/
		exheader->aci.alsc.storage_info.fsai[0] |= 1 << 7;
		// Mount sdmc:/ (Write Access)
		exheader->aci.aac.descriptors[1] |= 2;
		// Allow direct register access
		for(int i = 0; i < 28; ++i)
		{
			if(exheader->aci.akc.descriptors[i] == 0xFFFFFFFFu)
			{
				// hid
				exheader->aci.akc.descriptors[i] = 0xFFE00000u | ((0x10146000 + 0xEB00000) >> 12);
				// gpu
				exheader->aci.akc.descriptors[i + 1] = 0xFFE00000u | ((0x10400000 + 0xEB00000) >> 12);
				break;
			}
		}
	}
}

__attribute__((naked))
void readExHeaderHookHandler(int res)
{
	if(res >= 0)
	{
		ncch_ex_h* exheader;
		asm volatile
		(
			"stmfd sp!, {r0-r5, lr}\n\t"
			"ldr %0, [sp, #0x20]"
			:"=r"(exheader)
		);

		patchExHeader(exheader);

		asm volatile
		(
			"ldmfd sp!, {r0-r5, lr}\n\t"
			"add lr, lr, #0xA\n\t"
			"bx lr"
		);
	}
	else
	{
		void(*closeAndReturn)() = (void*) (a9compat.pm_pxi_readexheader + 0x35);
		asm volatile("mov r4, r0\n\t":::"r0","r4");
		closeAndReturn();
	}
}

void patchARM9()
{
	// pm_pxi_readexheader
	uint32_t offset = a9compat.pm_pxi_readexheader;
	*(uint32_t*)(offset + 0) = 0x47884900; // ldr r1, [pc, #0] -> blx r1
	*(uint32_t*)(offset + 4) = (uint32_t) readExHeaderHookHandler;
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

void patchARM11Kernel()
{
	a11compat.buffer = a9compat.buffer_va;
	a11compat.translate_va = a9compat.translate_va;
	a11compat.mmu_table_offset = a9compat.mmu_table_offset;
	a11compat.codeset_offset = a9compat.codeset_offset;
	a11compat.exit_process = a9compat.exit_process;

	memset((void*)a9compat.buffer_pa, 0, 0xC00);
	memcpy((void*)a9compat.hooks_pa, &_a11_hook_start, &_a11_hook_end - &_a11_hook_start);

	uint32_t a11_base_pa = a9compat.kernel_base_pa;
	uint32_t a11_base_va = a9compat.kernel_base_va;

	uint32_t offset = a9compat.data_abort_offset;
	*(uint32_t*)(a11_base_pa + offset + 8) = a9compat.hooks_va + ((uint8_t*)dataabort_hook - &_a11_hook_start);

	offset = a9compat.ld11_offset;
	*(uint32_t*)(a11_base_pa + offset) = getBranchInst(a11_base_va + offset, a9compat.hooks_va + ((uint8_t*)ld11_hook - &_a11_hook_start), 1);

	offset = a9compat.ssr_offset;
	if(offset) // Disable service monitor for firms with different prologue in svcSendSyncRequest handler
		*(uint32_t*)(a11_base_pa + offset) = getBranchInst(a11_base_va + offset, a9compat.hooks_va + ((uint8_t*)ssr_hook - &_a11_hook_start), 1);
}

void dumpAXIWRAM()
{
	dump_to_file("sdmc:/axiwram.bin", (void*)0x1FF80000, 0x80000);
}

void readARM11CodeBin(uint32_t* text, uint32_t size)
{
	// SVC Handler patch, allow all svcs
	*a9compat.svc_patch_pa = 0;
	// SVC handler for svc 0x3F
	*a9compat.svc_dev_patch_pa = a9compat.hooks_va + ((uint8_t*)svcDevHandler - &_a11_hook_start);;

	dump_to_mem("sdmc:/dhs.bin", text, 0);

	// Set buffer loc
	text[1] = a9compat.buffer_va;
}

void processA11Hooks()
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

void bgThreadEntry()
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

void doExploit()
{
	svcBackdoor(mpuSetupWrap);
	if(!dump_only)
	{
		patchARM9();
		patchARM11Kernel();
	}

	Handle thread;
	void* entry = (void*)bgThreadEntry;
	void* arg = NULL;
	void* stackTop = (void*)0x1FFAFB4;
	svcCreateThread(&thread, entry, arg, stackTop, 0x3F, -2);
}
