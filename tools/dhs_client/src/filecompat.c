#include "filecompat.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

ssize_t getFileSize(const char* fname)
{
	// https://www.securecoding.cert.org/confluence/display/c/FIO19-C.+Do+not+use+fseek%28%29+and+ftell%28%29+to+compute+the+size+of+a+regular+file
	struct stat statbuf;

	if(fname == NULL)
		return -1;

	if(stat(fname, &statbuf) != 0)
		return -1;

	return (ssize_t)statbuf.st_size;
}
