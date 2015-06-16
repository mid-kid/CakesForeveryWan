#include "dhs_patch/dhs_patch_compat.h"

#include "file.h"
#include <string.h>

extern dhs_a9_compat_s a9compat;

void char2wchar(wchar_t* dst, const char* src, int size)
{
	int i = 0;
	for(; i < size; ++i)
	{
		*dst++ = *src;

		if(*src++ == 0) break;
	}
}

int f_open(FILE* file, const char* path, uint32_t flags)
{
	wchar_t fname16[0x20];
	char2wchar(fname16, path, 0x20);
	return a9compat.IFile_Open(&file->file, fname16, flags);
}

void f_close(FILE* file)
{
	a9compat.IFile_Close(&file->file);
}

void f_getsize(FILE* file, uint64_t* size)
{
	a9compat.IFile_GetSize(&file->file, size);
}

void f_seek(FILE* file, uint64_t offset)
{
	file->file.fptr = offset;
}

void f_tell(FILE* file, uint64_t* offset)
{
	*offset = file->file.fptr;
}

size_t f_read(FILE* file, void* buffer, size_t size)
{
	uint32_t read = 0;
	a9compat.IFile_Read(&file->file, &read, buffer, size);
	
	return read;
}

size_t f_write(FILE* file, const void* buffer, size_t size)
{
	uint32_t written = 0;
	a9compat.IFile_Write(&file->file, &written, buffer, size, 1);
	
	return written;
}

int dump_to_mem(const char* path, void* buffer, size_t size)
{
	FILE file;
	memset(&file, 0, sizeof(file));
	f_open(&file, path, FILE_R);

	uint64_t size64 = size;
	if(size == 0)
		f_getsize(&file, &size64);

	uint32_t read = f_read(&file, buffer, size64);
	f_close(&file);

	return read;
}

int dump_to_file(const char* path, const void* buffer, size_t size)
{
	FILE file;
	memset(&file, 0, sizeof(file));
	f_open(&file, path, FILE_W);
	
	uint32_t written = f_write(&file, buffer, size);
	f_close(&file);

	return written;
}
