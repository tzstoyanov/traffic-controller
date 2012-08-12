#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_packets_api.h"
#include "tl_api.h"
#include "lib_tl_log.h"

int tl_mgmt_connect()
{
	int iSock, iOldFlags, iFlags;
	struct sockaddr_un stSockAddress;
	
	stSockAddress.sun_family = AF_UNIX;
	sprintf(stSockAddress.sun_path, "%s", TL_API_SOCKET_NAME);
	
	iSock = socket(PF_LOCAL, SOCK_STREAM, 0);
	if(-1 == iSock)
		return iSock;
	iOldFlags = fcntl( iSock, F_GETFL );
	iFlags  = iOldFlags | O_NONBLOCK;
	fcntl( iSock, F_SETFL, iFlags );
	
	if(0 == connect(iSock, (struct sockaddr*)&stSockAddress, sizeof(struct sockaddr_un )))
	{
		fcntl( iSock, F_SETFL, iOldFlags );
		return iSock;
	}
	if(errno != EINPROGRESS)
	{
		close(iSock);
		return -1;	
	}
	return -1;
}

int tl_mgmt_send(int iFd, uint8_t iCommand, uint8_t *pData, uint32_t iLen)
{
 int iRet;
 
	if(-1 == iFd)
		return ERROR;
	iRet = write(iFd, &iCommand, 1);
	if(1 != iRet)
		return ERROR;
	iRet = write(iFd, &iLen, 1);
	if(1 != iRet)
		return ERROR;
	if(iLen > 0)
	{	
		iRet = write(iFd, pData, iLen);
		if(iLen != iRet)
			return ERROR;
	}
	return OK;	
}

int tl_mgmt_get(int iFd, uint8_t *pData, uint32_t iLen)
{
	return read(iFd, pData, iLen);
}


