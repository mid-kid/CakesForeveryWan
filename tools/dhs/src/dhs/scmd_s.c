#include "scmd_s.h"
#include "3ds/types.h"
#include "3ds/services/soc.h"
#include "3ds/services/fs.h"
#include "3ds/services/am.h"
#include "3ds/svc.h"
#include "3ds/srv.h"
#include "3ds/os.h"

#include "svccust.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <string.h>

#include "dhs_patch/dhs_patch.h"
extern void* kbuffer;

extern uint32_t __heapBase;
extern uint32_t __linear_heap;

extern uint32_t pid;
extern uint32_t firmVersion;
uint32_t bdArgs[8];

static void* _kfind_kprocess(uint32_t namelo, uint32_t namehi);

static void _kget_kprocess()
{
	asm volatile("cpsid aif");
	bdArgs[0] = *((uint32_t*)0xFFFF9004);
}

static uint32_t kget_kprocess()
{
	svcDev(_kget_kprocess);
	return bdArgs[0];
}

static void _kmemcpy()
{
	asm volatile("cpsid aif");
	memcpy((void*)bdArgs[0], (void*)bdArgs[1], bdArgs[2]);
}

static void kmemcpy(void* buffer, void* addr, uint32_t size)
{
	bdArgs[0] = (uint32_t)buffer;
	bdArgs[1] = (uint32_t)addr;
	bdArgs[2] = size;
	svcDev(_kmemcpy);
}

static void _kstart_ssr()
{
	volatile ssr_data_s* data = (volatile ssr_data_s*) kbuffer;
	data->name_lo = bdArgs[0];
	data->name_hi = bdArgs[1];
	data->magic = HAXX;

	data->processed = 1;
}

static void kstart_ssr(const char* name)
{
	bdArgs[0] = *((uint32_t*)(name));
	bdArgs[1] = *((uint32_t*)(name + 4));
	svcDev(_kstart_ssr);
}

static void _kstop_ssr()
{
	volatile ssr_data_s* data = (volatile ssr_data_s*) kbuffer;
	data->name_lo = 0;
	data->name_hi = 0;
	data->magic = 0;

	data->processed = 1;
}

static void kstop_ssr()
{
	svcDev(_kstop_ssr);
}

static void _kprocess_ssr()
{
	volatile ssr_data_s* data = (volatile ssr_data_s*) kbuffer;

	uint32_t timeout = 0xFFFFFFFF;
	while(data->processed && --timeout);

	if(timeout != 0)
	{
		*(uint32_t*)bdArgs[0] = data->handle;
		memcpy((void*)bdArgs[1], (void*)data->cmd_buffer, 0x100);
		data->processed = 1;

		bdArgs[0] = 0;
	}
	else
		bdArgs[0] = 1;
}

static int kprocess_ssr(uint32_t* handle, void* buffer)
{
	bdArgs[0] = (uint32_t)handle;
	bdArgs[1] = (uint32_t)buffer;
	svcDev(_kprocess_ssr);

	return bdArgs[0];
}

static void* _kfind_kprocess(uint32_t namelo, uint32_t namehi)
{
	size_t size = 0;
	void* currKProcess = (void*)*((uint32_t*)0xFFFF9004);
	if(firmVersion < 0x022C0600) // Less than ver 8.0.0
		size = 0x260;
	else
		size = 0x268;

	void* offset = currKProcess - pid * size;
	for(int i = 0; *(uint32_t*)(offset + i * size + 4); i++)
	{
		void* toffset = offset + i * size;
		KCodeSet* codeset = (firmVersion < 0x022C0600) ? ((KProcess_4*)toffset)->kcodeset : ((KProcess_8*)toffset)->kcodeset;
		if(codeset->namelo == namelo && codeset->namehi == namehi)
			return toffset;
	}

	return NULL;
}

static uint32_t _ktranslate_va_pa(uint32_t* mmu_table, uint32_t addr)
{
	uint32_t taddr = 0;

	uint32_t table_offset = addr >> 20;
	uint32_t l1 = mmu_table[table_offset];
	if((l1 & 3) == 2)
	{
		if(l1 & (1 << 18)) // Supersection
			taddr = (l1 & 0xFF000000) | (addr & ~0xFF000000);
		else // Section
			taddr = (l1 & 0xFFF00000) | (addr & ~0xFFF00000);
	}
	else if((l1 & 3) == 1)
	{
		uint32_t* mmu_table_l2 = (uint32_t*)((l1 & 0xFFFFFC00) + ((firmVersion < 0x022C0600) ? 0xD0000000 : 0xC0000000));
		uint32_t table_offset = (addr << 12) >> 24;
		uint32_t l2 = mmu_table_l2[table_offset];

		if((l2 & 3) == 1) // Large page
			taddr = (l2 & 0xFFFF0000) | (addr & ~0xFFFF0000);
		else if((l2 & 3) >= 2) // Small page
			taddr = (l2 & 0xFFFFF000) | (addr & ~0xFFFFF000);
	}

	return taddr;
}

static uint32_t _ktranslate_pa_va_l2(uint32_t* mmu_table_l2, uint32_t addr, uint32_t table_l1_offset)
{
	uint32_t skip = 1;
	for(uint32_t table_l2_offset = 0; table_l2_offset < 256; table_l2_offset += skip, skip = 1)
	{
		// Page table
		uint32_t base = 0;
		uint32_t mask = 0;
		uint32_t size = 0;

		uint32_t l2 = mmu_table_l2[table_l2_offset];
		if(l2)
		{
			if((l2 & 3) == 1) // Large page
			{
				mask = 0xFFFF0000;
				base = l2 & mask;
				size = 16 * 4 * 1024;

				skip = 16;
			}
			else if((l2 & 3) >= 2) // Small page
			{
				mask = 0xFFFFF000;
				base = l2 & mask;
				size = 4 * 1024;
			}
		}

		if(mask && addr >= base && addr < base + size)
			return (table_l1_offset << 20) | (table_l2_offset << 12) | (addr & ~mask);
	}

	return 0;
}

static uint32_t _ktranslate_pa_va(uint32_t* mmu_table, uint32_t addr, uint32_t range)
{
	uint32_t taddr = 0;
	uint32_t skip = 1;
	for(uint32_t table_l1_offset = 0; table_l1_offset < range; table_l1_offset += skip, skip = 1)
	{
		uint32_t base = 0;
		uint32_t mask = 0;
		uint32_t size = 0;

		uint32_t l1 = mmu_table[table_l1_offset];
		if((l1 & 3) == 2)
		{
			if(l1 & (1 << 18)) // Supersection
			{
				mask = 0xFF000000;
				base = l1 & mask;
				size = 16 * 1024 * 1024;

				skip = 16;
			}
			else // Section
			{
				mask = 0xFFF00000;
				base = l1 & mask;
				size = 1024 * 1024;
			}

			if(mask && addr >= base && addr < base + size)
				return (table_l1_offset << 20) | (addr & ~mask);

			mask = 0;
		}
		else if((l1 & 3) == 1)
		{
			uint32_t* mmu_table_l2 = (uint32_t*)((l1 & 0xFFFFFC00) + ((firmVersion < 0x022C0600) ? 0xD0000000 : 0xC0000000));
			taddr = _ktranslate_pa_va_l2(mmu_table_l2, addr, table_l1_offset);

			if(taddr) return taddr;
		}
	}

	return 0;
}

static void _ktranslate()
{
	asm volatile("cpsid aif");

	uint32_t addr = bdArgs[0];
	uint32_t from = bdArgs[1];
	uint32_t to = bdArgs[2];
	uint32_t namelo = bdArgs[3];
	uint32_t namehi = bdArgs[4];

	uint32_t taddr = 0;
	uint32_t mmu_table_size = 0;
	uint32_t* mmu_table = 0;
	if(from == MEMTYPE_PROCESS || to == MEMTYPE_PROCESS)
	{
		void* kprocess = _kfind_kprocess(namelo, namehi);
		if(kprocess)
		{
			if(firmVersion < 0x022C0600) // Less than ver 8.0.0
			{
				mmu_table = (uint32_t*)((KProcess_4*)kprocess)->mmu_table;
				mmu_table_size = ((KProcess_4*)kprocess)->mmu_table_size;
			}
			else
			{
				mmu_table = (uint32_t*)((KProcess_8*)kprocess)->mmu_table;
				mmu_table_size = ((KProcess_8*)kprocess)->mmu_table_size;
			}
		}
	}
	else if(from == MEMTYPE_KERNEL || to == MEMTYPE_KERNEL)
	{
		mmu_table = (uint32_t*)(0x1FFF8000 + ((firmVersion < 0x022C0600) ? 0xD0000000 : 0xC0000000));
		mmu_table_size = 4096 * sizeof(uint32_t);
	}

	if(mmu_table)
	{
		if(to == MEMTYPE_PHYSICAL)
			taddr = _ktranslate_va_pa(mmu_table, addr);
		else if(from == MEMTYPE_PHYSICAL)
			taddr = _ktranslate_pa_va(mmu_table, addr, mmu_table_size / sizeof(uint32_t));
	}

	bdArgs[0] = taddr;
}

static uint32_t ktranslate(uint32_t addr, uint32_t from, uint32_t to, uint32_t namelo, uint32_t namehi)
{
	bdArgs[0] = addr;
	bdArgs[1] = from;
	bdArgs[2] = to;
	bdArgs[3] = namelo;
	bdArgs[4] = namehi;
	svcDev(_ktranslate);

	return bdArgs[0];
}

int32_t sGetInfo(scmdreq_s* cmd, int sockfd, void* buffer, uint32_t bufSize)
{
	scmdres_info_s scmdInfo;
	scmdInfo.res = 0;
	scmdInfo.kernelver = osGetKernelVersion();
	scmdInfo.firmver = osGetFirmVersion();

	scmdInfo.kernel_syscorever = *(uint32_t*)0x1FF80010;
	scmdInfo.firm_syscorever = *(uint32_t*)0x1FF80064;
	scmdInfo.unit_info = (*(uint32_t*)(0x1FF80014)) & 0xFF;
	scmdInfo.kernel_ctrsdkver = *(uint32_t*)0x1FF80018;
	scmdInfo.firm_ctrsdkver = *(uint32_t*)0x1FF80068;
	scmdInfo.app_memtype = *(uint32_t*)0x1FF80030;
	scmdInfo.app_memalloc = *(uint32_t*)0x1FF80040;
	scmdInfo.sys_memalloc = *(uint32_t*)0x1FF80044;
	scmdInfo.base_memalloc = *(uint32_t*)0x1FF80048;

	scmdInfo.pid = pid;
	scmdInfo.heap = __heapBase;
	scmdInfo.linheap = __linear_heap;
	scmdInfo.kprocess_addr = kget_kprocess();
	send(sockfd, &scmdInfo, sizeof(scmdInfo), 0);

	return 0;
}

int32_t sDump(scmdreq_dump_s* cmd, int sockfd, void* buffer, uint32_t bufSize)
{
	int32_t ret = -1;

	scmdres_dump_s* res = (scmdres_dump_s*) buffer;
	res->res = 0;
	res->size = cmd->size;
	uint32_t filled = sizeof(scmdres_dump_s);
	do
	{
		uint32_t size = bufSize - filled;
		size = size < cmd->size ? size : cmd->size;

		kmemcpy((uint8_t*)buffer + filled, cmd->addr + (res->size - cmd->size), size);
		send(sockfd, buffer, size + filled, 0);

		filled = 0;
		cmd->size -= size;
	} while(cmd->size != 0);

	return ret;
}

int32_t sPatch(scmdreq_patch_s* cmd, int sockfd, void* buffer, uint32_t bufSize)
{
	ssize_t bytesRead = 0;
	u32 totalSize = 0;
	do
	{
		bytesRead = recv(sockfd, buffer, bufSize, 0);
		if(bytesRead > 0)
		{
			kmemcpy((uint8_t*)cmd->addr + totalSize, buffer, bytesRead);

			totalSize += bytesRead;
		}
	} while(bytesRead > 0 && totalSize < cmd->size);

	scmdres_patch_s res;
	res.res = totalSize != cmd->size;
	send(sockfd, &res, sizeof(res), 0);

	return res.res;
}

int32_t sInstallCia(scmdreq_install_s* cmd, int sockfd, void* buffer, uint32_t bufSize)
{
	int32_t ret = -1;
	Handle ciaHandle;

	if((ret = AM_StartCiaInstall(cmd->media, &ciaHandle)) == 0)
	{
		ssize_t bytesRead = 0;
		u32 totalSize = 0;
		do
		{
			bytesRead = recv(sockfd, buffer, bufSize, 0);
			if(bytesRead > 0)
			{
				ret = FSFILE_Write(ciaHandle, NULL, totalSize, buffer, bytesRead, FS_WRITE_NOFLUSH);
				if(ret)
					break;

				totalSize += bytesRead;
			}
		} while(bytesRead > 0 && ret == 0 && totalSize < cmd->filesize);

		if(ret == 0 && totalSize == cmd->filesize)
		{
			ret = AM_FinishCiaInstall(cmd->media, &ciaHandle);
		}
		else
		{
			AM_CancelCIAInstall(&ciaHandle);
			if(ret == 0) ret = -1;
		}
	}

	scmdres_install_s res;
	res.res = ret;
	send(sockfd, &res, sizeof(res), 0);

	return ret;
}

int32_t sDeleteCia(scmdreq_delete_s* cmd, int sockfd, void* buffer, uint32_t bufSize)
{
	int32_t ret = -1;

	if((cmd->titleid >> 32) & 0xFFFF)
		ret = AM_DeleteTitle(cmd->media, cmd->titleid);
	else
		ret = AM_DeleteAppTitle(cmd->media, cmd->titleid);

	scmdres_delete_s res;
	res.res = ret;
	send(sockfd, &res, sizeof(res), 0);

	return ret;
}

int32_t sInstallFirm(scmdreq_s* cmd, int sockfd, void* buffer, uint32_t bufSize)
{
	int32_t ret = AM_InstallNativeFirm();

	scmdres_installfirm_s res;
	res.res = ret;
	send(sockfd, &res, sizeof(res), 0);

	return ret;
}

int32_t sTranslate(scmdreq_translate_s* cmd, int sockfd, void* buffer, uint32_t bufSize)
{
	int32_t ret = 0;

	scmdres_translate_s res;
	res.address = ktranslate(cmd->address, cmd->from, cmd->to, cmd->namelo, cmd->namehi);
	res.res = ret;
	send(sockfd, &res, sizeof(res), 0);

	return ret;
}

int32_t sGetHandle(scmdreq_gethandle_s* cmd, int sockfd, void* buffer, uint32_t bufSize)
{
	Handle handle;
	int32_t ret = srvGetServiceHandle(&handle, cmd->name);

	scmdres_gethandle_s res;
	res.res = ret;
	res.handle = handle;
	send(sockfd, &res, sizeof(res), 0);

	return ret;
}

int32_t sService(scmdreq_service_s* cmd, int sockfd, void* buffer, uint32_t bufSize)
{
	Result ret = 0;

	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = cmd->header_code;
	for(uint32_t i = 0; i < cmd->argc; ++i)
	{
		cmdbuf[i + 1] = cmd->argv[i];
	}

	ret = svcSendSyncRequest(cmd->handle);

	scmdres_service_s* res = (scmdres_service_s*) buffer;
	res->res = ret;
	res->size = cmd->output_size;

	uint32_t filled = sizeof(scmdres_service_s);
	uint32_t left = cmd->output_size;
	do
	{
		uint32_t size = bufSize - filled;
		size = size < cmd->output_size ? size : left;

		memcpy((uint8_t*)buffer + filled, (u8*)cmdbuf + (res->size - left), size);
		send(sockfd, buffer, size + filled, 0);

		filled = 0;
		left -= size;
	} while(left != 0);

	return ret;
}

int32_t sServiceMon(scmdreq_servicemon_s* cmd, int sockfd, void* buffer, uint32_t bufSize)
{
	if(kbuffer == 0)
		return -1;

	scmdres_servicemon_s* res = (scmdres_servicemon_s*) buffer;
	res->res = 0;

	kstart_ssr(cmd->name);
	do
	{
		if(kprocess_ssr(&res->handle, res->cmd_buffer) == 0)
		{
			if(send(sockfd, buffer, sizeof(scmdres_servicemon_s), 0) == -1)
				break;
		}
		else
		{
			svcSleepThread(200000);
		}
	} while(1);

	kstop_ssr();

	return 0;
}
