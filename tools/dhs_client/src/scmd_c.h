#ifndef __SCMD_C_H_
#define __SCMD_C_H_

int cGetInfo(int sockfd, void* buffer, size_t bufSize);
int cDump(int sockfd, void* buffer, size_t bufSize, void* addr, size_t size, const char* fname, void* dbuffer);
int cPatch(int sockfd, void* buffer, size_t bufSize, void* addr, size_t size, const char* fname, uint32_t value, uint32_t value_set);
int cInstallCia(int sockfd, void* buffer, size_t bufSize, const char* fname, uint32_t mediatype);
int cDeleteCia(int sockfd, void* buffer, size_t bufSize, uint64_t titleid, uint32_t mediatype);
int cInstallFirm(int sockfd, void* buffer, size_t bufSize);
int cTranslate(int sockfd, void* buffer, size_t bufSize, void* addr, uint32_t from, uint32_t to, const char* process);
int cGetHandle(int sockfd, void* buffer, size_t bufSize, const char* name);
int cService(int sockfd, void* buffer, size_t bufSize, uint32_t handle, uint32_t headerCode, uint32_t argc, uint32_t* argv, uint32_t outputSize);

int cScreenshot(int sockfd, void* buffer, size_t bufSize, const char* fname);

#endif /*__SCMD_C_H_*/
