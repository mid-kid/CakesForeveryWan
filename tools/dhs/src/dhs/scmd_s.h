#ifndef __SCMD_S_H_
#define __SCMD_S_H_

#include "dhs/scmd.h"

int32_t sGetInfo(scmdreq_s* cmd, int sockfd, void* buffer, uint32_t bufSize);
int32_t sDump(scmdreq_dump_s* cmd, int sockfd, void* buffer, uint32_t bufSize);
int32_t sPatch(scmdreq_patch_s* cmd, int sockfd, void* buffer, uint32_t bufSize);
int32_t sInstallCia(scmdreq_install_s* cmd, int sockfd, void* buffer, uint32_t bufSize);
int32_t sDeleteCia(scmdreq_delete_s* cmd, int sockfd, void* buffer, uint32_t bufSize);
int32_t sInstallFirm(scmdreq_s* cmd, int sockfd, void* buffer, uint32_t bufSize);

#endif /*__SCMD_S_H_*/
