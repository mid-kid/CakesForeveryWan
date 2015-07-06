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

typedef struct dump_ctx
{
	uint32_t processed;
	uint32_t size;
	void* data;
} dump_ctx;

int dumpFileCb(void* buffer, uint32_t size, dump_ctx* data)
{
	data->processed += size;
	return fwrite(buffer, size, 1, (FILE*)(data->data)) != 1;
}

int dumpMemCb(void* buffer, uint32_t size, dump_ctx* data)
{
	memcpy(data->data + data->processed, buffer, size);
	data->processed += size;

	return 0;
}

int dumpPrintCb(void* buffer, uint32_t size, dump_ctx* data)
{
	if(data->processed == 0)
		fprintf(stdout, "0x%08X : ", (uint32_t)data->data);

	for(int i = 0; i < size; i++, data->processed++)
	{
		fprintf(stdout, "%02X ", ((uint8_t*)buffer)[i]);

		uint32_t total = data->processed + 1;
		if(total % 16 == 0 && total != (data->size))
			fprintf(stdout, "\n0x%08X : ", (uint32_t)data->data + total);
	}

	return 0;
}

int cDump(int sockfd, void* buffer, size_t bufSize, void* addr, size_t size, const char* fname, void* dbuffer)
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

		dump_ctx ctx;
		ctx.size = cmd.size;
		ctx.processed = 0;

		int(*dumpCb)(void*, uint32_t, dump_ctx*) = NULL;

		if(fname)
		{
			FILE* file = fopen(fname, "wb");
			if(file)
			{
				ctx.data = file;
				dumpCb = dumpFileCb;
			}
		}
		else if(dbuffer)
		{
			ctx.data = dbuffer;
			dumpCb = dumpMemCb;
		}
		else
		{
			ctx.data = addr;
			dumpCb = dumpPrintCb;
		}

		if(dumpCb)
		{
			uint32_t left = size;

			do
			{
				if(readBytes)
				{
					dumpCb(buffer + filled, readBytes, &ctx);
					left -= readBytes;
					readBytes = filled = 0;
				}
				if(left)
				{
					readBytes = recv(sockfd, buffer, bufSize, 0);
					if(readBytes <= 0)
						break;
				}
			} while(left && readBytes);
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
	fprintf(stdout, " mediatype : %s\n", mediatype == 0 ? "NAND" : "SD");

	ssize_t readBytes = 0;
	FILE* file = fopen(fname, "rb");
	if(file)
	{
		fseek(file, 0, SEEK_END);
		size_t size = ftell(file);

		scmdreq_install_s cmd;
		cmd.req.magic = SCMD_MAGIC;
		cmd.req.cmd = SCMD_INSTALL;
		cmd.media = mediatype;
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

int cTranslate(int sockfd, void* buffer, size_t bufSize, void* addr, uint32_t from, uint32_t to, const char* process)
{
	if(from > MEMTYPE_PHYSICAL || to > MEMTYPE_PHYSICAL)
		return -1;
	if((from == MEMTYPE_PROCESS || to == MEMTYPE_PROCESS) && process == NULL)
		return -1;
	if(from != MEMTYPE_PHYSICAL && to != MEMTYPE_PHYSICAL) // no user va to kernel va or vice versa, yet
		return -1;
	if(from == to) // ...
		return -1;

	fprintf(stdout, "Translate address\n");
	fprintf(stdout, " input : 0x%08X\n", (uint32_t)addr);

	scmdreq_translate_s cmd;
	cmd.req.magic = SCMD_MAGIC;
	cmd.req.cmd = SCMD_TRANSLATE;
	cmd.address = (uint32_t)addr;
	cmd.from = from;
	cmd.to = to;
	if(process != NULL)
	{
		char namebuf[9];
		memset(namebuf, 0, 9);
		sprintf(namebuf, "%s", process);

		cmd.namehi = *(uint32_t*)(namebuf);
		cmd.namelo = *(uint32_t*)(namebuf + 4);

		printf("%08X%08X\n", cmd.namehi, cmd.namelo);

		fprintf(stdout, " process : %s\n", process);
	}

	send(sockfd, &cmd, sizeof(cmd), 0);
	if(readAtLeast(sockfd, buffer, bufSize, sizeof(scmdres_translate_s)) == sizeof(scmdres_translate_s))
	{
		printResponse(((scmdres_translate_s*)buffer)->res);
		fprintf(stdout, "Address : 0x%08X\n", ((scmdres_translate_s*)buffer)->address);
	}
	else
	{
		fprintf(stderr, "No response from server\n");
		exit(-1);
	}

	return 0;
}

int cGetHandle(int sockfd, void* buffer, size_t bufSize, const char* name)
{
	if(strlen(name) > 8)
		return -1;

	fprintf(stdout, "Get handle\n");

	scmdreq_gethandle_s cmd;
	cmd.req.magic = SCMD_MAGIC;
	cmd.req.cmd = SCMD_GETHANDLE;
	memset(cmd.name, 0, sizeof(cmd.name));
	strncpy(cmd.name, name, 8);

	send(sockfd, &cmd, sizeof(cmd), 0);
	if(readAtLeast(sockfd, buffer, bufSize, sizeof(scmdres_gethandle_s)) == sizeof(scmdres_gethandle_s))
	{
		printResponse(((scmdres_gethandle_s*)buffer)->res);
		fprintf(stdout, " Handle : 0x%08X\n", ((scmdres_gethandle_s*)buffer)->handle);
	}
	else
	{
		fprintf(stderr, "No response from server\n");
		exit(-1);
	}

	return 0;
}

int cService(int sockfd, void* buffer, size_t bufSize, uint32_t handle, uint32_t headerCode, uint32_t argc, uint32_t* argv, uint32_t outputSize)
{
	if(!handle)
		return -1;
	if(!headerCode)
		return -1;

	fprintf(stdout, "Service\n");
	fprintf(stdout, "Handle : 0x%08X\n", handle);
	fprintf(stdout, " 0x%08X\n", headerCode);

	scmdreq_service_s cmd;
	cmd.req.magic = SCMD_MAGIC;
	cmd.req.cmd = SCMD_SERVICE;
	cmd.handle = handle;
	cmd.header_code = headerCode;
	cmd.argc = argc;
	cmd.output_size = outputSize;
	for(uint32_t i = 0; i < argc; ++i)
	{
		cmd.argv[i] = argv[i];
		fprintf(stdout, " 0x%08X\n", argv[i]);
	}

	send(sockfd, &cmd, sizeof(cmd), 0);
	ssize_t readBytes = readAtLeast(sockfd, buffer, bufSize, sizeof(scmdres_service_s));
	scmdres_service_s* res = (scmdres_service_s*)buffer;
	if(res->res == 0)
	{
		uint32_t filled = sizeof(scmdres_dump_s);
		readBytes -= filled;

		dump_ctx ctx;
		ctx.size = cmd.output_size;
		ctx.processed = 0;

		int(*dumpCb)(void*, uint32_t, dump_ctx*) = dumpPrintCb;

		uint32_t left = outputSize;
		do
		{
			if(readBytes)
			{
				dumpCb(buffer + filled, readBytes, &ctx);
				left -= readBytes;
				readBytes = filled = 0;
			}
			if(left)
			{
				readBytes = recv(sockfd, buffer, bufSize, 0);
				if(readBytes <= 0)
					break;
			}
		} while(left && readBytes);
	}
	else
	{
		printResponse(((scmdres_service_s*)buffer)->res);
	}

	return 0;
}

typedef struct lcd_fb_setup
{
	uint16_t height;			// 0x5C
	uint16_t width;				// 0x5E
	uint8_t pad0[0x68 - 0x60];
	void* fb_left1;				// 0x68
	void* fb_left2;
	uint32_t format;
	uint32_t pad1;
	uint32_t select;			// 0x78
	uint8_t pad2[0x90 - 0x7C];
	uint32_t stride;			// 0x90
	void* fb_right1;			// 0x94
	void* fb_right2;
} lcd_fb_setup;

#pragma pack(push, 2)

typedef struct dib_h
{
	uint32_t header_size;
	uint32_t width;
	uint32_t height;
	uint16_t plane;
	uint16_t bits;
	uint32_t comp;
	uint32_t data_size;
	uint32_t junk[4];
} dib_h;

typedef struct bm_h
{
	uint16_t magic;
	uint32_t file_size;
	uint32_t pad;
	uint32_t offset;
	dib_h dib;
} bm_h;

#pragma pack(pop)

int cScreenshot(int sockfd, void* buffer, size_t bufSize, const char* fname)
{
	int res = -1;
	if(fname == NULL)
		return -1;

	if((res = cTranslate(sockfd, buffer, bufSize, (void*)0x10400400, MEMTYPE_PHYSICAL, MEMTYPE_KERNEL, NULL)) != 0)
		return 1;

	void* lcd_fb_setup_top_addr = (void*)((scmdres_translate_s*)buffer)->address + 0x5C;
	void* lcd_fb_setup_sub_addr = (void*)((scmdres_translate_s*)buffer)->address + 0x15C;

	lcd_fb_setup top_setup;
	lcd_fb_setup sub_setup;
	if((res = cDump(sockfd, buffer, bufSize, lcd_fb_setup_top_addr, sizeof(lcd_fb_setup), NULL, &top_setup)) != 0)
		return 1;
	if((res = cDump(sockfd, buffer, bufSize, lcd_fb_setup_sub_addr, sizeof(lcd_fb_setup), NULL, &sub_setup)) != 0)
		return 1;

	if((res = cTranslate(sockfd, buffer, bufSize, top_setup.fb_left1, MEMTYPE_PHYSICAL, MEMTYPE_KERNEL, NULL)) != 0)
		return 1;
	void* fb_top_left_addr = (void*)((scmdres_translate_s*)buffer)->address;

	if((res = cTranslate(sockfd, buffer, bufSize, top_setup.fb_right1, MEMTYPE_PHYSICAL, MEMTYPE_KERNEL, NULL)) != 0)
		return 1;
	void* fb_top_right_addr = (void*)((scmdres_translate_s*)buffer)->address;

	if((res = cTranslate(sockfd, buffer, bufSize, sub_setup.fb_left1, MEMTYPE_PHYSICAL, MEMTYPE_KERNEL, NULL)) != 0)
		return 1;
	void* fb_sub_addr = (void*)((scmdres_translate_s*)buffer)->address;

	char tfname[0x100];
	void* bmpBuffer = malloc(1024 * 1024);
	bm_h* bm = (bm_h*) bmpBuffer;

	memset(bm, 0, sizeof(bm_h));
	bm->magic = 0x4D42;
	bm->offset = sizeof(bm_h);
	bm->dib.header_size = sizeof(dib_h);
	bm->dib.plane = 1;
	bm->dib.bits = 24;

	sprintf(tfname, "%s-top-l.bmp", fname);
	bm->dib.width = top_setup.height;
	bm->dib.height = -top_setup.width;
	bm->dib.data_size = top_setup.width * top_setup.height * 3;
	bm->file_size = sizeof(bm_h) + bm->dib.data_size;
	if((res = cDump(sockfd, buffer, bufSize, fb_top_left_addr, bm->dib.data_size, NULL, bmpBuffer + sizeof(bm_h))) != 0)
		return 1;

	FILE* bm_file = fopen(tfname, "wb");
	if(bm_file)
	{
		fwrite(bmpBuffer, bm->file_size, 1, bm_file);
		fclose(bm_file);
	}

	sprintf(tfname, "%s-top-r.bmp", fname);
	bm->dib.width = top_setup.height;
	bm->dib.height = -top_setup.width;
	bm->dib.data_size = top_setup.width * top_setup.height * 3;
	bm->file_size = sizeof(bm_h) + bm->dib.data_size;
	if((res = cDump(sockfd, buffer, bufSize, fb_top_right_addr, bm->dib.data_size, NULL, bmpBuffer + sizeof(bm_h))) != 0)
		return 1;

	bm_file = fopen(tfname, "wb");
	if(bm_file)
	{
		fwrite(bmpBuffer, bm->file_size, 1, bm_file);
		fclose(bm_file);
	}

	sprintf(tfname, "%s-sub.bmp", fname);
	bm->dib.width = sub_setup.height;
	bm->dib.height = -sub_setup.width;
	bm->dib.data_size = sub_setup.width * sub_setup.height * 3;
	bm->file_size = sizeof(bm_h) + bm->dib.data_size;
	if((res = cDump(sockfd, buffer, bufSize, fb_sub_addr, bm->dib.data_size, NULL, bmpBuffer + sizeof(bm_h))) != 0)
		return 1;

	bm_file = fopen(tfname, "wb");
	if(bm_file)
	{
		fwrite(bmpBuffer, bm->file_size, 1, bm_file);
		fclose(bm_file);
	}

	return 0;
}
