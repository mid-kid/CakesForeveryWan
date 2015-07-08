#include "dhs_patch/dhs_patch_compat.h"

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

int df_open(DFILE* file, const char* path, uint32_t flags)
{
	wchar_t fname16[0x20];
	char2wchar(fname16, path, 0x20);
	return a9compat.IFile_Open(&file->file, fname16, flags);
}

void df_close(DFILE* file)
{
	a9compat.IFile_Close(&file->file);
}

void df_getsize(DFILE* file, uint64_t* size)
{
	a9compat.IFile_GetSize(&file->file, size);
}

void df_seek(DFILE* file, uint64_t offset)
{
	file->file.fptr = offset;
}

void df_tell(DFILE* file, uint64_t* offset)
{
	*offset = file->file.fptr;
}

size_t df_read(DFILE* file, void* buffer, size_t size)
{
	uint32_t read = 0;
	a9compat.IFile_Read(&file->file, &read, buffer, size);

	return read;
}

size_t df_write(DFILE* file, const void* buffer, size_t size)
{
	uint32_t written = 0;
	a9compat.IFile_Write(&file->file, &written, buffer, size, 1);

	return written;
}

int dump_to_mem(const char* path, void* buffer, size_t size)
{
	DFILE file;
	memset(&file, 0, sizeof(file));
	df_open(&file, path, FILE_R);

	uint64_t size64 = size;
	if(size == 0)
		df_getsize(&file, &size64);

	uint32_t read = df_read(&file, buffer, size64);
	df_close(&file);

	return read;
}

int dump_to_file(const char* path, const void* buffer, size_t size)
{
	DFILE file;
	memset(&file, 0, sizeof(file));
	df_open(&file, path, FILE_W);

	uint32_t written = df_write(&file, buffer, size);
	df_close(&file);

	return written;
}
