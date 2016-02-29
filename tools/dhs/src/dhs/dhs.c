#include "3ds/types.h"
#include "3ds/srv.h"
#include "3ds/svc.h"
#include "3ds/services/fs.h"
#include "3ds/services/am.h"
#include "3ds/services/soc.h"
#include "3ds/services/ac.h"
#include "3ds/os.h"
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#include "dhs/scmd.h"
#include "dhs/kobj.h"
#include "scmd_s.h"

#define DATA_BUFFER_SIZE (512 * 1024)
void __system_allocateHeaps();
extern u32 __heapBase;
extern u32 __heap_size;

#define DIE(x, y) *(u32*)x = y

u32 pid;
int listenfd;
u32 firmVersion;

Handle notificationSem = 0;

void initVfp()
{
	asm volatile
	(
		"mov r0, #0x3000000\n\t"
		"vmsr fpscr, r0\n\t" 		// NaN mode enabled. flush-to-zero mode enabled
		:::"r0"
	);
}

__attribute__((naked))
void svcDev(void(*fun)())
{
	asm volatile
	(
		"svc 0x3F\n\t"
		"bx lr"
	);
}

Result startServer()
{
	Result ret;
	if((ret = socInit((u32*) (__heapBase + (__heap_size - 0x48000)), 0x48000)))
		DIE(0x14000040, ret);

	struct sockaddr_in serv_addr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd < 0)
		DIE(0x14000044, listenfd);

	int flags = fcntl(listenfd, F_GETFL, 0);
	fcntl(listenfd, F_SETFL, flags | O_NONBLOCK);

	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(8333);

	if((ret = bind(listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr))) != 0)
		DIE(0x14000048, listenfd);

	if((ret = listen(listenfd, 10)) != 0)
		DIE(0x1400004C, listenfd);

	return ret;
}

int snprintf ( char * s, size_t n, const char * format, ... )
{
	// Stub to decrease size : TODO
}

ssize_t readAtLeast(int sockfd, void* buffer, size_t bufSize, size_t size)
{
	ssize_t bytesRead = 0;
	ssize_t totalRead = 0;
	do
	{
		bytesRead = recv(sockfd, buffer + totalRead, bufSize - totalRead, 0);
		if(bytesRead <= 0)
			break;

		totalRead += bytesRead;
	} while(totalRead < size);

	return (bytesRead != -1 && totalRead >= size) ? totalRead : -1;
}

Result readFullCmd(int sockfd, void* buffer, size_t bufSize, size_t bytesRead, size_t cmdSize)
{
	ssize_t nBytesRead = 0;
	if(bytesRead < cmdSize)
	{
		nBytesRead = readAtLeast(sockfd, buffer + bytesRead, bufSize, cmdSize - bytesRead);
	}

	return nBytesRead < 0;
}

void acceptAndServe()
{
	int sockfd = accept(listenfd, (struct sockaddr*) NULL, NULL);
	if(sockfd < 0)
	{
		if(errno != EAGAIN || errno != EWOULDBLOCK)
			DIE(0x14000050, sockfd);
		else
			return;
	}

	int flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags & ~O_NONBLOCK);

	const u32 bufSize = __heap_size - 0x48000 - 0x200;
	u8* buffer = (u8*)(__heapBase + 0x200);

	ssize_t bytesRead = 0;
	do
	{
		scmdack_s ack;
		ack.magic = SCMD_MAGIC;

		Result res = -1;
		bytesRead = readAtLeast(sockfd, buffer, bufSize, sizeof(scmdreq_s));
		if(bytesRead >= sizeof(scmdreq_s))
		{
			scmdreq_s* scmd = (scmdreq_s*) buffer;
			if(scmd->magic == SCMD_MAGIC)
			{
				switch(scmd->cmd)
				{
				case SCMD_INFO:
					res = sGetInfo(scmd, sockfd, buffer, bufSize);
					break;
				case SCMD_DUMP:
					if(readFullCmd(sockfd, buffer, bufSize, bytesRead, sizeof(scmdreq_dump_s)) == 0)
					{
						scmdreq_dump_s scmdDump;
						memcpy(&scmdDump, scmd, sizeof(scmdreq_dump_s));
						res = sDump(&scmdDump, sockfd, buffer, bufSize);
					}
					break;
				case SCMD_PATCH:
					if(readFullCmd(sockfd, buffer, bufSize, bytesRead, sizeof(scmdreq_patch_s)) == 0)
					{
						send(sockfd, &ack, sizeof(ack), 0);

						scmdreq_patch_s scmdPatch;
						memcpy(&scmdPatch, scmd, sizeof(scmdreq_patch_s));
						res = sPatch(&scmdPatch, sockfd, buffer, bufSize);
					}
					break;
				case SCMD_INSTALL:
					if(readFullCmd(sockfd, buffer, bufSize, bytesRead, sizeof(scmdreq_install_s)) == 0)
					{
						send(sockfd, &ack, sizeof(ack), 0);

						scmdreq_install_s scmdInstall;
						memcpy(&scmdInstall, scmd, sizeof(scmdreq_install_s));
						res = sInstallCia(&scmdInstall, sockfd, buffer, bufSize);
					}
					break;
				case SCMD_DELETE:
					if(readFullCmd(sockfd, buffer, bufSize, bytesRead, sizeof(scmdreq_delete_s)) == 0)
					{
						scmdreq_delete_s scmdDelete;
						memcpy(&scmdDelete, scmd, sizeof(scmdreq_delete_s));
						res = sDeleteCia(&scmdDelete, sockfd, buffer, bufSize);
					}
					break;
				case SCMD_INSTALLFIRM:
					res = sInstallFirm(scmd, sockfd, buffer, bufSize);
					break;
				case SCMD_TRANSLATE:
					if(readFullCmd(sockfd, buffer, bufSize, bytesRead, sizeof(scmdreq_translate_s)) == 0)
					{
						scmdreq_translate_s scmdTranslate;
						memcpy(&scmdTranslate, scmd, sizeof(scmdreq_translate_s));
						res = sTranslate(&scmdTranslate, sockfd, buffer, bufSize);
					}
					break;
				case SCMD_GETPROCESS_LIST:
					res = sGetProcessList(scmd, sockfd, buffer, bufSize);
					break;
				case SCMD_GETKPROCESS:
					if(readFullCmd(sockfd, buffer, bufSize, bytesRead, sizeof(scmdreq_getkprocess_s)) == 0)
					{
						scmdreq_getkprocess_s scmdGetKProcess;
						memcpy(&scmdGetKProcess, scmd, sizeof(scmdreq_getkprocess_s));
						res = sGetKProcess(&scmdGetKProcess, sockfd, buffer, bufSize);
					}
					break;
				case SCMD_GETHANDLE:
					if(readFullCmd(sockfd, buffer, bufSize, bytesRead, sizeof(scmdreq_gethandle_s)) == 0)
					{
						scmdreq_gethandle_s scmdGetHandle;
						memcpy(&scmdGetHandle, scmd, sizeof(scmdreq_gethandle_s));
						res = sGetHandle(&scmdGetHandle, sockfd, buffer, bufSize);
					}
					break;
				case SCMD_SERVICE:
					if(readFullCmd(sockfd, buffer, bufSize, bytesRead, sizeof(scmdreq_service_s)) == 0)
					{
						scmdreq_service_s scmdService;
						memcpy(&scmdService, scmd, sizeof(scmdreq_service_s));
						res = sService(&scmdService, sockfd, buffer, bufSize);
					}
					break;
				case SCMD_SERVICEMON:
					if(readFullCmd(sockfd, buffer, bufSize, bytesRead, sizeof(scmdreq_servicemon_s)) == 0)
					{
						scmdreq_servicemon_s scmdServiceMon;
						memcpy(&scmdServiceMon, scmd, sizeof(scmdreq_servicemon_s));
						res = sServiceMon(&scmdServiceMon, sockfd, buffer, bufSize);
					}
					break;
				default:
					send(sockfd, &ack, sizeof(ack), 0);
					break;
				}
			}
		}
	} while(bytesRead >= 0);

	closesocket(sockfd);
}

void init()
{
	svcSleepThread(5000000000LL);

	Result ret;
	if((ret = fsInit()) != 0)
		DIE(0x14000004, ret);

	if((ret = amInit()) != 0)
		DIE(0x14000010, ret);

	if((ret = acInit()) != 0)
		DIE(0x14000010, ret);

	srvEnableNotification(&notificationSem);
	srvSubscribe(0x100);
}

void deInit()
{
	if(notificationSem)
		svcCloseHandle(notificationSem);

	srvExit();

	acExit();
	amExit();
	fsExit();
}

void patchPid()
{
	asm volatile("cpsid aif");

	if(firmVersion < 0x022C0600) // Less than ver 8.0.0
	{
		KProcess_4* kProcess = *(KProcess_4**)0xFFFF9004;
		kProcess->pid = 0;
	}
	else
	{
		KProcess_8* kProcess = *(KProcess_8**)0xFFFF9004;
		kProcess->pid = 0;
	}
}

void unpatchPid()
{
	asm volatile("cpsid aif");

	if(firmVersion < 0x022C0600) // Less than ver 8.0.0
	{
		KProcess_4* kProcess = *(KProcess_4**)0xFFFF9004;
		kProcess->pid = pid;
	}
	else
	{
		KProcess_8* kProcess = *(KProcess_8**)0xFFFF9004;
		kProcess->pid = pid;
	}
}

int main()
{
	Handle handle;
	asm volatile
	(
		"ldr r1,=0xFFFF8001\n\t"
		"svc 0x27\n\t"				// Dup handle
		"mov %0, r1"
		: "=r"(handle)
	);

	for(u32 addr = 0x100000; addr < 0x10C000; addr += 0x1000)
		svcControlProcessMemory(handle, addr, 0, 0x1000, MEMOP_PROT, MEMPERM_READ | MEMPERM_WRITE | MEMPERM_EXECUTE);

	svcSleepThread(5000000000LL);

	svcGetProcessId(&pid, 0xFFFF8001);
	firmVersion = osGetFirmVersion();

	svcDev(patchPid);

	Result ret;
	if((ret = srvInit()) != 0)
		DIE(0x14000000, ret);

	svcDev(unpatchPid);

	__system_allocateHeaps();
	initVfp();
	init();

	if((ret = acWaitInternetConnection()) != 0)
		DIE(0x14000018, ret);

	startServer();
	while(1)
	{
		acceptAndServe();

		u32 nId;
		srvReceiveNotification(&nId);
		if(nId == 0x100)
			break;

		svcSleepThread(0x2FAF080);
	}

	svcExitProcess();

	return 0;
}
