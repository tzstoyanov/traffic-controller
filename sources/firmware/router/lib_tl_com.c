#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "lib_tl_log.h"
#include "tl_serial_api.h"
#include "tl_packets_api.h"
#include "tl_controlers_api.h"
#include "lib_tl_config.h"
#include "lib_tl_packets.h"

int tl_com_send(tl_com_t *pCom)
{
	uint32_t iRet, iSize;
	
	iSize = strlen(pCom->arrSend);
	iRet = write(pCom->iNetFd, pCom->arrSend, iSize);
	if(iRet != iSize)
		return ERROR;
	return OK;	
}

int tl_com_local_pkt_verify(tl_com_t *pCom)
{
	if(pCom->iRcvSize != pCom->iSendSize || 0 != memcmp(pCom->arrSend, pCom->arrRecived, pCom->iRcvSize))
	{
		tl_log_error("Local Echo check failed !");
		pCom->iLineSate = TL_COM_IDLE;
		return ERROR;
	}
	if(TL_COM_LOCAL_VERIFY_WAIT == pCom->iLineSate)
		pCom->iLineSate = TL_COM_PKT_WAIT;
	else
		pCom->iLineSate = TL_COM_IDLE;
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
	if(1 == read(pCom->iNetFd, pBuf, 1))
		return OK;
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
			bBegin=1;

		if( bBegin && pCom->iRcvSize < (TL_MAX_PACKET_SERIAL_SIZE-1) )
		{
			pCom->arrRecived[pCom->iRcvSize] = iChar;
			if(TL_PKT_END == iChar)
				break;
			pCom->iRcvSize++;	
		}
		iRet = tl_com_read_symbol(pCom, &iChar);
	}
	
	if(TL_PKT_END == pCom->arrRecived[pCom->iRcvSize] && OK == iRet)
	{	
		pCom->arrRecived[pCom->iRcvSize+1] = 0;
		if(TL_IS_DEBUG(RX))
		{
			tl_log_info("Recived packet from %s, line state is %d", 
				pCom->szPortName, pCom->iLineSate );
			if(TL_IS_DEBUG(PKT_DUMP))
				tl_log_info("[%s]", pCom->arrRecived);
		}
	
		switch(pCom->iLineSate)
		{
			case TL_COM_LOCAL_VERIFY:
			case TL_COM_LOCAL_VERIFY_WAIT:
				tl_com_local_pkt_verify(pCom);
				break;
			case TL_COM_IDLE:				
			case TL_COM_PKT_WAIT:
				tl_com_pkt_process(pCom);
				break;
			default:
				break;
		}
	}
	else
	{
		tl_log_error("Received broken packet");
		pCom->iLineSate = TL_COM_IDLE;
		iRet = ERROR;
	}
	pCom->iRcvSize = 0;
	pCom->arrRecived[0]=0;
	return iRet;
}

int tl_com_sym_open(tl_com_t *pCom)
{
	int iOn=1;
	struct sockaddr_un stSockAddress;
	
	stSockAddress.sun_family = AF_UNIX;
	sprintf(stSockAddress.sun_path, "%s", pCom->szPortName);
	pCom->iNetFd = socket(PF_LOCAL,SOCK_DGRAM, 0);
	if(0 > pCom->iNetFd)
		return ERROR;
	setsockopt(pCom->iNetFd, SOL_SOCKET, SO_REUSEADDR, (void *) &iOn, sizeof(iOn));
	
	unlink(stSockAddress.sun_path);
	
	if( 0 > bind(pCom->iNetFd, (struct sockaddr*)&stSockAddress, sizeof(struct sockaddr_un)))
		return ERROR;

	chmod(stSockAddress.sun_path, 
		S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
	
	pCom->iLineSate = TL_COM_IDLE;
		
	if(TL_IS_DEBUG(INIT))
		tl_log_info("Com port %s is ready", pCom->szPortName);
		
	return OK;
}

