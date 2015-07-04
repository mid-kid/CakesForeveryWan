#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include "dhs/scmd.h"
#include "socket.h"
#include "scmd_c.h"

enum DHSC_CMD
{
	DHSC_INFO = 1,
	DHSC_DUMP,
	DHSC_PATCH,
	DHSC_EXAMINE,
	DHSC_INSTALL,
	DHSC_DELETE,
	DHSC_INSTALLFIRM,
	DHSC_TRANSLATE
};

int connectToServer(const char* host, const char* port)
{
	socketStartup();

	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_protocol = IPPROTO_TCP;

	int res = getaddrinfo(host, port, &hints, &result);
	if(res != 0)
	{
		fprintf(stderr, "getaddrinfo failed with error: %d\n", res);
		socketCleanup();
		exit(-1);
	}

	for(struct addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if(sockfd == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", socketGetError());
			socketCleanup();
			exit(-1);
		}

		res = connect(sockfd, ptr->ai_addr, (int) ptr->ai_addrlen);
		if(res == SOCKET_ERROR)
		{
			close(sockfd);
			sockfd = INVALID_SOCKET;
			continue;
		}

		break;
	}

	freeaddrinfo(result);

	if(sockfd == INVALID_SOCKET)
	{
		printf("Unable to connect to server!\n");
		socketCleanup();
		exit(-1);
	}

	return sockfd;
}

void printHelp(char* name)
{
	fprintf(stderr, "Usage %s hostname command\n", name);
	fprintf(stderr, "Commands:\n");
	fprintf(stderr, "  info - Print server info\n");
	fprintf(stderr, "  dump - Dump memory\n");
	fprintf(stderr, "    -addr\t\t Virtual address to dump\n");
	fprintf(stderr, "    -size\t\t Dump size\n");
	fprintf(stderr, "    -file\t\t Output file (optional)\n");
	fprintf(stderr, "  patch - Patch memory\n");
	fprintf(stderr, "    -addr\t\t Virtual address to patch\n");
	fprintf(stderr, "    -size\t\t Patch size (ignored when no input file is specified)\n");
	fprintf(stderr, "    -file\t\t Input file (optional)\n");
	fprintf(stderr, "    -value\t\t Value to patch to (optional)\n");
	fprintf(stderr, "  install - Install cia\n");
	fprintf(stderr, "    -file\t\t Input file\n");
	fprintf(stderr, "    -mediatype\t\t Media type, 0 : NAND, 1 : SD\n");
	fprintf(stderr, "  delete - Delete application\n");
	fprintf(stderr, "    -titleid\t\t Title id\n");
	fprintf(stderr, "    -mediatype\t\t Media type, 0 : NAND, 1 : SD\n");
	fprintf(stderr, "  installfirm - Perform AM:InstallNATIVEFIRM\n");
	fprintf(stderr, "  translate - Translate virtual address to physical address\n");
	fprintf(stderr, "    -addr\t\t Virtual address to translate\n");
	fprintf(stderr, "    -from\t\t 0 : Kernel, 1 : Process\n");
	fprintf(stderr, "    -process\t\t If \"from\" is 1, the name of the process table to use\n");
}

typedef struct input_s
{
	uint32_t cmd;

	const char* fname;
	const char* process;
	uint32_t mediatype;
	uint64_t titleid;
	void* addr;
	uint32_t size;
	uint32_t from;
	uint32_t to;
	uint32_t value;
	uint32_t value_set;
} input_s;

int parseArgs(input_s* input, int argc, char *argv[])
{
	input->mediatype = -1;

	if(strcmp(argv[0], "info") == 0)
		input->cmd = DHSC_INFO;
	else if(strcmp(argv[0], "dump") == 0)
		input->cmd = DHSC_DUMP;
	else if(strcmp(argv[0], "patch") == 0)
		input->cmd = DHSC_PATCH;
	else if(strcmp(argv[0], "install") == 0)
		input->cmd = DHSC_INSTALL;
	else if(strcmp(argv[0], "delete") == 0)
		input->cmd = DHSC_DELETE;
	else if(strcmp(argv[0], "installfirm") == 0)
		input->cmd = DHSC_INSTALLFIRM;
	else if(strcmp(argv[0], "translate") == 0)
		input->cmd = DHSC_TRANSLATE;
	else
		return -1;

	for(int i = 1; i < argc; ++i)
	{
		if(strcmp(argv[i], "-file") == 0 && (i + 1) < argc)
			input->fname = argv[i + 1];
		if(strcmp(argv[i], "-process") == 0 && (i + 1) < argc)
			input->process = argv[i + 1];
		else if(strcmp(argv[i], "-addr") == 0 && (i + 1) < argc)
			input->addr = (void*)strtoul(argv[i + 1], NULL, 0);
		else if(strcmp(argv[i], "-size") == 0 && (i + 1) < argc)
			input->size = strtoul(argv[i + 1], NULL, 0);
		else if(strcmp(argv[i], "-from") == 0 && (i + 1) < argc)
			input->from = strtoul(argv[i + 1], NULL, 0);
		else if(strcmp(argv[i], "-to") == 0 && (i + 1) < argc)
			input->to = strtoul(argv[i + 1], NULL, 0);
		else if(strcmp(argv[i], "-mediatype") == 0 && (i + 1) < argc)
			input->mediatype = strtoul(argv[i + 1], NULL, 0);
		else if(strcmp(argv[i], "-titleid") == 0 && (i + 1) < argc)
			input->titleid = strtoull(argv[i + 1], NULL, 16);
		else if(strcmp(argv[i], "-value") == 0 && (i + 1) < argc)
		{
			input->value = strtoul(argv[i + 1], NULL, 0);
			input->value_set = 1;
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	if(argc < 3)
	{
		printHelp(argv[0]);
		exit(0);
	}

	input_s input;
	memset(&input, 0, sizeof(input));

	if(parseArgs(&input, argc - 2, &argv[2]) || input.cmd == 0)
	{
		printHelp(argv[0]);
		exit(0);
	}

	int sockfd = connectToServer(argv[1], "8333");
	int res = -1;
	if(sockfd > 0)
	{
		fprintf(stdout, "Connected to server: %s\n", argv[1]);

		size_t bufSize = 1024 * 1024;
		void* buffer = malloc(bufSize);

		switch(input.cmd)
		{
		case DHSC_INFO:
			res = cGetInfo(sockfd, buffer, bufSize);
			break;
		case DHSC_DUMP:
			res = cDump(sockfd, buffer, bufSize, input.addr, input.size, input.fname);
			break;
		case DHSC_PATCH:
			res = cPatch(sockfd, buffer, bufSize, input.addr, input.size, input.fname, input.value, input.value_set);
			break;
		case DHSC_INSTALL:
			res = cInstallCia(sockfd, buffer, bufSize, input.fname, input.mediatype);
			break;
		case DHSC_DELETE:
			res = cDeleteCia(sockfd, buffer, bufSize, input.titleid, input.mediatype);
			break;
		case DHSC_INSTALLFIRM:
			res = cInstallFirm(sockfd, buffer, bufSize);
			break;
		case DHSC_TRANSLATE:
			res = cTranslate(sockfd, buffer, bufSize, input.addr, input.from, input.process);
			break;
		default:
			break;
		}

		if(res)
		{
			printHelp(argv[0]);
			exit(0);
		}

		close(sockfd);
	}

	return 0;
}
