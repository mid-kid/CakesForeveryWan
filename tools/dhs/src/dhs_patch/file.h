#ifndef __FILE_H
#define __FILE_H

#include <stdint.h>
#include <stddef.h>

#define FILE_R 0x1
#define FILE_W 0x6

typedef struct IFILE
{
	uint32_t unk[4];
	uint64_t fptr;
	uint64_t size;
} IFILE;

typedef struct FILE
{
	IFILE file;
} FILE;

int f_open(FILE* file, const char* path, uint32_t flags);
void f_close(FILE* file);
void f_getsize(FILE* file, uint64_t* size);
void f_seek(FILE* file, uint64_t offset);
void f_tell(FILE* file, uint64_t* offset);

size_t f_read(FILE* file, void* buffer, size_t size);
size_t f_write(FILE* file, const void* buffer, size_t size);

/**Utility functions**/
int dump_to_mem(const char* path, void* buffer, size_t size);
int dump_to_file(const char* path, const void* buffer, size_t size);

int(*IFile_Open)(IFILE* file, const wchar_t* path, uint32_t flags);
void(*IFile_Close)(IFILE* file);
void(*IFile_GetSize)(IFILE* file, uint64_t* size);

void(*IFile_Read)(IFILE* file, uint32_t* read, void* buffer, uint32_t size);
void(*IFile_Write)(IFILE* file, uint32_t* written, const void* buffer, uint32_t size, uint32_t flush);

#endif // __FILE_H
