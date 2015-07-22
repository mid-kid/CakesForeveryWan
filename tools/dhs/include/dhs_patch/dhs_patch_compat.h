#ifndef __DHS_PATCH_COMPAT_H
#define __DHS_PATCH_COMPAT_H

#include <stdint.h>
#include <stddef.h>

typedef struct IFILE IFILE;

typedef struct dhs_a9_compat_s
{
	uint32_t hook_load;
	uint32_t hook_return;

	uint32_t buffer_pa;
	uint32_t buffer_va;
	uint32_t hooks_pa;
	uint32_t hooks_va;
	uint32_t kernel_base_pa;
	uint32_t kernel_base_va;

	uint32_t data_abort_offset;
	uint32_t ld11_offset;

	uint32_t* svc_patch_pa;
	uint32_t* svc_dev_patch_pa;

	void*(*translate_va)(void* mmu_vt, void* addr_va);
	uint32_t mmu_table_offset;
	uint32_t codeset_offset;
	uint32_t exit_process;

	int(*IFile_Open)(IFILE* file, const wchar_t* path, uint32_t flags);
	void(*IFile_Close)(IFILE* file);
	void(*IFile_GetSize)(IFILE* file, uint64_t* size);
	void(*IFile_Read)(IFILE* file, uint32_t* read, void* buffer, uint32_t size);
	void(*IFile_Write)(IFILE* file, uint32_t* written, const void* buffer, uint32_t size, uint32_t flush);
} dhs_a9_compat_s;

typedef struct dhs_a11_compat_s
{
	uint32_t buffer;
	void*(*translate_va)(void* mmu_vt, void* addr_va);
	uint32_t mmu_table_offset;
	uint32_t codeset_offset;
	uint32_t exit_process;
} dhs_a11_compat_s;

#ifdef DHS_PATCH_COMPAT_DATA

// Firmware 4.x
dhs_a9_compat_s dhs_a9_compat_1F = {
	.hook_load = 0x080CB388,
	.hook_return = 0x08087254,

	.buffer_pa = 0x1FFDF000,
	.buffer_va = 0xFFF3F000,
	.hooks_pa = 0x1FFF4B10,
	.hooks_va = 0xFFFF0B10,
	.kernel_base_pa = 0x1FF80000,
	.kernel_base_va = 0xFFF60000,
	.svc_patch_pa = (uint32_t*)0x1FF827CC,
	.svc_dev_patch_pa = (uint32_t*)0x1FF82968,

	.data_abort_offset = 0x348,
	.ld11_offset = 0x8344,

	.translate_va = (void*)0xFFF7A8C4,
	.mmu_table_offset = 0x54,
	.codeset_offset = 0xA8,
	.exit_process = 0xFFF72FCC,

	.IFile_Open = (void*)0x0805CF05,
	.IFile_Close = (void*)0x0805CFC5,
	.IFile_GetSize = (void*)0x0805DEF5,
	.IFile_Read = (void*)0x0804E315,
	.IFile_Write = (void*)0x0805E181,
};

// Firmware 6.1
dhs_a9_compat_s dhs_a9_compat_2A = {
    .hook_load = 0x080CAB68,
    .hook_return = 0x08085C84,

    .buffer_pa = 0x1FFDD000,
    .buffer_va = 0xFFF3D000,
    .hooks_pa = 0x1FFF4B10,
    .hooks_va = 0xFFFF0B10,
    .kernel_base_pa = 0x1FF80000,
    .kernel_base_va = 0xFFF50000,
    .svc_patch_pa = (uint32_t*)0x1FF822A4, // 0xFFF522A4
    .svc_dev_patch_pa = (uint32_t*)0x1FF82440,

    .data_abort_offset = 0x638,
    .ld11_offset = 0x7FBC,

    .translate_va = (void*)0xFFF6B7D0,
    .mmu_table_offset = 0x54,
    .codeset_offset = 0xA8,
    .exit_process = 0xFFF63820,

    .IFile_Open = (void*)0x0805AF89,
    .IFile_Close = (void*)0x0805B0D1,
    .IFile_GetSize = (void*)0x0805C18D,
    .IFile_Read = (void*)0x0804D8D9,
    .IFile_Write = (void*)0x0805C397,
};

dhs_a9_compat_s dhs_a9_compat_38 = {
	.hook_load = 0x080CB028,
	.hook_return = 0x0808605C,

	.buffer_pa = 0x1FFDD000,
	.buffer_va = 0xFFFAD000,
	.hooks_pa = 0x1FFF4B40,//0x1FFF4B30 0x1FFF4C30
	.hooks_va = 0xFFFF0B40,//0xFFFF0B30 0xFFFF0C30
	.kernel_base_pa = 0x1FF80000,
	.kernel_base_va = 0xFFF00000,
	.svc_patch_pa = (uint32_t*)0x1FF82290, // 0xFFF02290
	.svc_dev_patch_pa = (uint32_t*)0x1FF8242C, // 0xFFF0242C

	.data_abort_offset = 0x634,
	.ld11_offset = 0x82E8,

	.translate_va = (void*)0xFFF1BE04,
	.mmu_table_offset = 0x5C,
	.codeset_offset = 0xB0,
	.exit_process = 0xFFF13DFC,

	.IFile_Open = (void*)0x0805AF21,
	.IFile_Close = (void*)0x0805B00D,
	.IFile_GetSize = (void*)0x0805C119,
	.IFile_Read = (void*)0x0804D829,
	.IFile_Write = (void*)0x0805C31D,
};

// Firmware 9.6
dhs_a9_compat_s dhs_a9_compat_49 = {
	.hook_load = 0x080CAFA8,
	.hook_return = 0x08086144,

	.buffer_pa = 0x1FFDD000,
	.buffer_va = 0xFFFAD000,
	.hooks_pa = 0x1FFF4B40,//0x1FFF4B30 0x1FFF4C30
	.hooks_va = 0xFFFF0B40,//0xFFFF0B30 0xFFFF0C30
	.kernel_base_pa = 0x1FF80000,
	.kernel_base_va = 0xFFF00000,
	.svc_patch_pa = (uint32_t*)0x1FF82284, // 0xFFF02284
	.svc_dev_patch_pa = (uint32_t*)0x1FF82420,

	.data_abort_offset = 0x620,
	.ld11_offset = 0x82F4,

	.translate_va = (void*)0xFFF1C140,
	.mmu_table_offset = 0x5C,
	.codeset_offset = 0xB0,
	.exit_process = 0xFFF13E8C,

	.IFile_Open = (void*)0x0805B181,
	.IFile_Close = (void*)0x0805B26D,
	.IFile_GetSize = (void*)0x0805C2CD,
	.IFile_Read = (void*)0x0804D9B1,
	.IFile_Write = (void*)0x0805C4D1,
};

#endif /*DHS_PATCH_COMPAT_DATA*/

#endif /*__DHS_PATCH_COMPAT_H*/
