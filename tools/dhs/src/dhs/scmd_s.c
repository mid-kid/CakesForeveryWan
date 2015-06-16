#include "scmd_s.h"
#include "3ds/types.h"
#include "3ds/services/soc.h"
#include "3ds/services/fs.h"
#include "3ds/services/am.h"
#include "3ds/svc.h"
#include "3ds/os.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <string.h>

extern uint32_t __heapBase;
extern uint32_t __linear_heap;

extern uint32_t pid;
uint32_t bdArgs[4];

static void _kget_kprocess()
{
	asm volatile("cpsid aif");
	bdArgs[0] = *((uint32_t*)0xFFFF9004);
}

static uint32_t kget_kprocess()
{
	svcBackdoor((void*)_kget_kprocess);
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
	svcBackdoor((void*)_kmemcpy);
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

		kmemcpy((uint8_t*)buffer + filled, cmd->addr, size);
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
