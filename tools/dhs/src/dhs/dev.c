#include "dev.h"
#include "3ds/svc.h"
#include "3ds/srv.h"

// NOT USED REMOVE !

/*! PXIDEV handle */
static Handle devHandle;

#define NONE 0x454E4F4E
#define SRVC 0x43565253

Result devInit()
{
	return srvGetServiceHandle(&devHandle, "pxi:dev");
}

Handle* devGetSessionHandle()
{
	return &devHandle;
}

Result DEV_Initialize()
{
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x40;
	cmdbuf[1] = NONE;

	Result ret = 0;
	if((ret = svcSendSyncRequest(devHandle)))
		return ret;

	return cmdbuf[1];
}

Result DEV_Unk()
{
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x100;
	cmdbuf[1] = SRVC;
	cmdbuf[2] = SRVC;
	cmdbuf[3] = SRVC;
	cmdbuf[4] = SRVC;

	Result ret = 0;
	if((ret = svcSendSyncRequest(devHandle)))
		return ret;

	return cmdbuf[1];
}
