#include "dhs/scmd.h"
#include "errstr.h"

#include <stdio.h>
#include <stdlib.h>

#include <stddef.h>
#include <unistd.h>
#include "socket.h"

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

int dumpToSocket(int sockfd, void* buffer, size_t bufSize, FILE* file, size_t size)
{
	fseek(file, 0, SEEK_SET);
	size_t count;
	do
	{
		ssize_t readBytes = bufSize < size ? bufSize : size;
		count = fread(buffer, readBytes, 1, file);
		if(!count)
			break;

		send(sockfd, buffer, readBytes, 0);
		size -= readBytes;
	} while(count && size);

	return size != 0;
}

void printResponse(uint32_t res)
{
	const char* descStr = NULL;
	const char* summaryStr = NULL;
	const char* moduleStr = NULL;
	const char* levelStr = NULL;

	errstr(res, &descStr, &summaryStr, &moduleStr, &levelStr);
	fprintf(stdout, "Response : 0x%08X\n", res);
	fprintf(stdout, "Desc     : %s\n", descStr);
	fprintf(stdout, "Summary  : %s\n", summaryStr);
	fprintf(stdout, "Module   : %s\n", moduleStr);
	fprintf(stdout, "Level    : %s\n", levelStr);
}

int cGetInfo(int sockfd, void* buffer, size_t bufSize)
{
	fprintf(stdout, "Retrieving info\n");

	scmdreq_s cmd;
	cmd.magic = SCMD_MAGIC;
	cmd.cmd = SCMD_INFO;

	send(sockfd, &cmd, sizeof(cmd), 0);

	ssize_t readBytes = readAtLeast(sockfd, buffer, bufSize, sizeof(scmdres_info_s));

	scmdres_info_s* res = (scmdres_info_s*)buffer;
	fprintf(stdout, "Response : 0x%08X\n", res->res);
	fprintf(stdout, "  pid : 0x%08X\n", res->pid);
	fprintf(stdout, "  kernelver : 0x%08X\n", res->kernelver);
	fprintf(stdout, "  firmver : 0x%08X\n", res->firmver);

	fprintf(stdout, "  kernel_syscorever : 0x%08X\n", res->kernel_syscorever);
	fprintf(stdout, "  firm_syscorever : 0x%08X\n", res->firm_syscorever);
	fprintf(stdout, "  unit_info : 0x%08X\n", res->unit_info);
	fprintf(stdout, "  kernel_ctrsdkver : 0x%08X\n", res->kernel_ctrsdkver);
	fprintf(stdout, "  firm_ctrsdkver : 0x%08X\n", res->firm_ctrsdkver);
	fprintf(stdout, "  app_memtype : 0x%08X\n", res->app_memtype);
	fprintf(stdout, "  app_memalloc : 0x%08X\n", res->app_memalloc);
	fprintf(stdout, "  sys_memalloc : 0x%08X\n", res->sys_memalloc);
	fprintf(stdout, "  base_memalloc : 0x%08X\n", res->base_memalloc);

	fprintf(stdout, "  heap : 0x%08X\n", res->heap);
	fprintf(stdout, "  linheap : 0x%08X\n", res->linheap);
	fprintf(stdout, "  KProcess* : 0x%08X\n", res->kprocess_addr);

	return 0;
}

int cDump(int sockfd, void* buffer, size_t bufSize, void* addr, size_t size, const char* fname)
{
	if(addr == NULL || size == 0)
		return -1;

	fprintf(stdout, "Dumping\n");

	scmdreq_dump_s cmd;
	cmd.req.magic = SCMD_MAGIC;
	cmd.req.cmd = SCMD_DUMP;
	cmd.addr = addr;
	cmd.is_pa = 0;
	cmd.size = size;

	send(sockfd, &cmd, sizeof(cmd), 0);

	ssize_t readBytes = readAtLeast(sockfd, buffer, bufSize, sizeof(scmdres_dump_s));

	scmdres_dump_s* res = (scmdres_dump_s*)buffer;
	if(res->res == 0)
	{
		uint32_t filled = sizeof(scmdres_dump_s);
		readBytes -= filled;

		if(fname)
		{
			FILE* file = fopen(fname, "wb");
			if(file)
			{
				do
				{
					if(readBytes)
					{
						fwrite(buffer + filled, readBytes, 1, file);
						readBytes = filled = 0;
						size -= readBytes;
					}
					if(size)
					{
						readBytes = recv(sockfd, buffer, bufSize, 0);
						if(readBytes <= 0)
							break;
					}

				} while(size && readBytes);

				fclose(file);
			}
			else
			{
				fprintf(stderr, "Failed to open file : %s\n", fname);
				exit(-1);
			}
		}
		else
		{
			size_t total = 1;
			fprintf(stdout, "\n0x%08X : ", addr);
			do
			{
				if(readBytes)
				{
					for(int i = 0; i < readBytes; i++, total++)
					{
						fprintf(stdout, "%02X ", ((uint8_t*)buffer + filled)[i]);

						if(total % 16 == 0 && total != (cmd.size))
							fprintf(stdout, "\n0x%08X : ", addr + total);
					}
					readBytes = filled = 0;
					size -= readBytes;
				}
				if(size)
				{
					readBytes = recv(sockfd, buffer, bufSize, 0);
					if(readBytes < 0)
						break;
				}
			} while(size && readBytes);
		}
	}

	return 0;
}

int cPatch(int sockfd, void* buffer, size_t bufSize, void* addr, size_t size, const char* fname, uint32_t value, uint32_t value_set)
{
	if(addr == NULL || (fname == NULL && !value_set))
		return -1;

	fprintf(stdout, "Patching\n");

	FILE* file = NULL;
	if(fname)
	{
		file = fopen(fname, "rb");
		if(file)
		{
			if(size == 0)
			{
				fseek(file, 0, SEEK_END);
				size = ftell(file);
			}
		}
		else
		{
			fprintf(stderr, "Failed to open file : %s\n", fname);
			exit(-1);
		}
	}

	scmdreq_patch_s cmd;
	cmd.req.magic = SCMD_MAGIC;
	cmd.req.cmd = SCMD_PATCH;
	cmd.addr = addr;
	cmd.is_pa = 0;
	cmd.size = size;

	send(sockfd, &cmd, sizeof(cmd), 0);

	ssize_t readBytes = readAtLeast(sockfd, buffer, bufSize, sizeof(scmdack_s));
	scmdack_s* ack = (scmdack_s*) buffer;
	if(readBytes < 0 || ack->magic != SCMD_MAGIC)
	{
		fprintf(stderr, "Invalid response from server, size : %d\n", readBytes);
		exit(-1);
	}

	if(file)
	{
		dumpToSocket(sockfd, buffer, bufSize, file, size);
		fclose(file);
	}
	else
	{
		send(sockfd, &value, 4, 0);
	}

	if(readAtLeast(sockfd, buffer, bufSize, sizeof(scmdres_patch_s)) == sizeof(scmdres_patch_s))
	{
		printResponse(((scmdres_patch_s*)buffer)->res);
	}
	else
	{
		fprintf(stderr, "No response from server\n");
		exit(-1);
	}

	return 0;
}

int cInstallCia(int sockfd, void* buffer, size_t bufSize, const char* fname, uint32_t mediatype)
{
	if(fname == NULL || mediatype > 1) // Only for NAND and SD
		return -1;

	fprintf(stdout, "Installing: %s\n", fname);

	ssize_t readBytes = 0;
	FILE* file = fopen(fname, "rb");
	if(file)
	{
		fseek(file, 0, SEEK_END);
		size_t size = ftell(file);

		scmdreq_install_s cmd;
		cmd.req.magic = SCMD_MAGIC;
		cmd.req.cmd = SCMD_INSTALL;
		cmd.media = 1;
		cmd.filesize = size;

		send(sockfd, &cmd, sizeof(cmd), 0);

		readBytes = readAtLeast(sockfd, buffer, bufSize, sizeof(scmdack_s));
		scmdack_s* ack = (scmdack_s*) buffer;
		if(readBytes < 0 || ack->magic != SCMD_MAGIC)
		{
			fprintf(stderr, "Invalid response from server, size : %d\n", readBytes);
			exit(-1);
		}

		dumpToSocket(sockfd, buffer, bufSize, file, size);

		fclose(file);
	}
	else
	{
		fprintf(stderr, "Failed to open cia : %s\n", fname);
		exit(-1);
	}

	fprintf(stdout, "File sent to server\n");

	readBytes = readAtLeast(sockfd, buffer, bufSize, sizeof(scmdres_install_s));
	if(readBytes == sizeof(scmdres_install_s))
	{
		printResponse(((scmdres_install_s*)buffer)->res);
	}
	else
	{
		fprintf(stderr, "No response from server\n");
		exit(-1);
	}

	return 0;
}

int cDeleteCia(int sockfd, void* buffer, size_t bufSize, uint64_t titleid, uint32_t mediatype)
{
	if(titleid == 0 || mediatype > 1) // Only for NAND and SD
		return -1;

	fprintf(stdout, "Deleting : %016llX\n", titleid);
	fprintf(stdout, " mediatype : %s\n", mediatype == 0 ? "NAND" : "SD");

	scmdreq_delete_s cmd;
	cmd.req.magic = SCMD_MAGIC;
	cmd.req.cmd = SCMD_DELETE;
	cmd.titleid = titleid;
	cmd.media = mediatype;

	send(sockfd, &cmd, sizeof(cmd), 0);
	if(readAtLeast(sockfd, buffer, bufSize, sizeof(scmdres_delete_s)) == sizeof(scmdres_delete_s))
	{
		printResponse(((scmdres_delete_s*)buffer)->res);
	}
	else
	{
		fprintf(stderr, "No response from server\n");
		exit(-1);
	}

	return 0;
}

int cInstallFirm(int sockfd, void* buffer, size_t bufSize)
{
	fprintf(stdout, "Install firm\n");

	scmdreq_s cmd;
	cmd.magic = SCMD_MAGIC;
	cmd.cmd = SCMD_INSTALLFIRM;

	send(sockfd, &cmd, sizeof(cmd), 0);
	if(readAtLeast(sockfd, buffer, bufSize, sizeof(scmdres_installfirm_s)) == sizeof(scmdres_installfirm_s))
	{
		printResponse(((scmdres_installfirm_s*)buffer)->res);
	}
	else
	{
		fprintf(stderr, "No response from server\n");
		exit(-1);
	}

	return 0;
}
