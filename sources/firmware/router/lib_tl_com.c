#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_packets_api.h"
#include "tl_serial_api.h"
#include "tl_program_api.h"
#include "tl_controlers_api.h"
#include "lib_tl_config.h"
#include "lib_tl_packets.h"
#include "lib_tl_log.h"

int tl_com_send(tl_com_t *pCom)
{
	int32_t iRet;
	fd_set stFdSet;
	struct timeval stTime;

	if(pCom->iNetFd < 0 )
		return ERROR;
	stTime.tv_sec = pCom->iTimeout / 1000;
	stTime.tv_usec = 1000*(pCom->iTimeout % 1000);
	FD_ZERO(&stFdSet);		
	FD_SET (pCom->iNetFd, &stFdSet);
	iRet = select (FD_SETSIZE, NULL, &stFdSet, NULL, &stTime);
	if(0 >= iRet)
		return ERROR;
	iRet = write(pCom->iNetFd, pCom->arrSend, pCom->iSendSize);
	if(iRet != pCom->iSendSize)
		return ERROR;

	return OK;	
}

int tl_com_read_symbol(tl_com_t *pCom, uint8_t *pBuf)
{
	int iRet;	
	fd_set stFdSet;
	struct timeval stTime;

	stTime.tv_sec = pCom->iTimeout / 1000;
	stTime.tv_usec = 1000*(pCom->iTimeout % 1000);
	FD_ZERO(&stFdSet);		
	FD_SET (pCom->iNetFd, &stFdSet);
	iRet = select (FD_SETSIZE, &stFdSet, NULL, NULL, &stTime);
	if(0 >= iRet)
		return ERROR;
	iRet = 	read(pCom->iNetFd, pBuf, 1);
	if(1 == iRet)
		return OK;
	if(0 == iRet)
	{
		close(pCom->iNetFd);
		pCom->iNetFd = -1;	
	}
	return ERROR;
}

int tl_com_read_packet(tl_com_t *pCom)
{
	int iRet;
	uint8_t iChar, bBegin=0;

	iRet = tl_com_read_symbol(pCom, &iChar);
	while(OK == iRet)
	{
		if(TL_PKT_START == iChar)
		{
			bBegin=1;
			pCom->iRcvSize = 0;
		}
		if( bBegin && pCom->iRcvSize < (TL_MAX_PACKET_SERIAL_SIZE-1) )
		{
			pCom->arrRecived[pCom->iRcvSize++] = iChar;
			if(TL_PKT_END == iChar)
			{
				pCom->arrRecived[pCom->iRcvSize] = 0; 
				if(TL_IS_DEBUG(RX) && TL_IS_DEBUG(PKT_DUMP))
					tl_log_info("Got packet [%s] %d bytes, going to verify it", 
								pCom->arrRecived, pCom->iRcvSize);
				break;
			}
		}
		else
		{
			if(TL_IS_DEBUG(RX))
				tl_log_error("Cannot find useable packet, reset buffer");
			pCom->iRcvSize = 0;
		}
		iRet = tl_com_read_symbol(pCom, &iChar);
	}
	return iRet;
}

int tl_com_sim_connect(tl_com_t *pCom)
{
	struct sockaddr_un stSockAddress;
	
	stSockAddress.sun_family = AF_UNIX;
	sprintf(stSockAddress.sun_path, "%s", pCom->szPortName);
	pCom->iNetFd = socket(PF_LOCAL,SOCK_STREAM, 0);
	if(0 > pCom->iNetFd)
		return ERROR;
	if( 0 > connect(pCom->iNetFd, (struct sockaddr*)&stSockAddress, sizeof(struct sockaddr_un)))
	{
		if(errno != EINPROGRESS)
		{
			close(pCom->iNetFd);
			pCom->iNetFd = -1;
		}
		return ERROR;	
	}
	
	pCom->iLineState = TL_COM_IDLE;
		
	if(TL_IS_DEBUG(INIT))
		tl_log_info("Com port %s is ready", pCom->szPortName);
		
	return OK;
}

int tl_com_sim_open(tl_com_t *pCom)
{
	int iOn=1;
	struct sockaddr_un stSockAddress;
	
	stSockAddress.sun_family = AF_UNIX;
	sprintf(stSockAddress.sun_path, "%s", pCom->szPortName);
	pCom->iAccFd = socket(PF_LOCAL,SOCK_STREAM, 0);
	if(0 > pCom->iAccFd)
		return ERROR;
	setsockopt(pCom->iAccFd, SOL_SOCKET, SO_REUSEADDR, (void *) &iOn, sizeof(iOn));
	
	unlink(stSockAddress.sun_path);
	
	if( 0 > bind(pCom->iAccFd, (struct sockaddr*)&stSockAddress, sizeof(struct sockaddr_un)))
		return ERROR;
	if(0 > listen(pCom->iAccFd, 1))
		return ERROR;

	chmod(stSockAddress.sun_path, 
		S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
	
	pCom->iLineState = TL_COM_IDLE;
		
	if(TL_IS_DEBUG(INIT))
		tl_log_info("Com port %s is ready", pCom->szPortName);
		
	return OK;
}

