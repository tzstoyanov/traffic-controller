#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_api.h"
#include "tl_serial.h"
#include "tl_program.h"
#include "tl_packets.h"
#include "tl_controlers.h"
#include "tl_config.h"
#include "tl_log.h"

int tl_com_local_pkt_verify(tl_com_t *pCom)
{
	if(pCom->iRcvSize != pCom->iSendSize || 0 != memcmp(pCom->arrSend, pCom->arrRecived, pCom->iRcvSize))
		tl_log_error("Local Echo check failed !");
	if(TL_COM_LOCAL_VERIFY_WAIT == pCom->iLineSate)
		pCom->iLineSate = TL_COM_PKT_WAIT;
	else
		pCom->iLineSate = TL_COM_IDLE;
	return OK;
}

int tl_com_read_packet(tl_com_t *pCom)
{
	uint8_t iChar;
	int iRet;

	iRet = read(pCom->iNetFd, &iChar, 1);
	while(1 == iRet)
	{
		if( pCom->iRcvSize < TL_MAX_PACKET_SERIAL_SIZE && 
			TL_PKT_END != pCom->arrRecived[pCom->iRcvSize] )
		{
			pCom->arrRecived[pCom->iRcvSize++] = iChar;
		}
		if(TL_PKT_END == pCom->arrRecived[pCom->iRcvSize])
			break;
		iRet = read(pCom->iNetFd, &iChar, 1);	
	}	
	
	if(TL_PKT_END == pCom->arrRecived[pCom->iRcvSize])
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
			case TL_COM_PKT_WAIT:
				tl_com_pkt_process(pCom);
				break;
			case TL_COM_IDLE:
			default:
				break;
		}
	}

	pCom->iRcvSize = 0;
	pCom->arrRecived[0]=0;
	return OK;
}

int32_t tl_com_read(thread_t *pThread)
{
	tl_global_t *pGlobal = tl_get_global();	
	tl_com_t *pCom = (tl_com_t*) pThread->pArg;
	
	pCom->pReadThread  = NULL;
	
	if(TL_IS_DEBUG(RX))
		tl_log_info("Recived unexpected data from %s, line state is %d", 
				pCom->szPortName, pCom->iLineSate );	
	
	tl_com_read_packet(pCom);
	pCom->pReadThread  = thread_read_add(&(pGlobal->stMaintThread), tl_com_read, pCom, pCom->iNetFd);
	return 1;
}

int tl_com_send(tl_com_t *pCom)
{
	uint32_t iRet, iSize;
	
	iSize = strlen(pCom->arrSend);
	iRet = write(pCom->iNetFd, pCom->arrSend, iSize);
	if(iRet != iSize)
		return ERROR;
	return OK;	
}

int tl_com_send_session(tl_com_t *pCom, uint8_t iAddress, uint8_t iNextState)
{
	tl_global_t *pGlobal = tl_get_global();
	fd_set stFdSet;
	int iRet;	
	struct timeval stTime;
	
	if(OK != tl_com_send(pCom))
		return ERROR;
	pCom->iLineSate = iNextState;
	stTime.tv_sec = pGlobal->iTimeout / 1000;
	stTime.tv_usec = 1000*(pGlobal->iTimeout % 1000);
	FD_ZERO(&stFdSet);		
	FD_SET (pCom->iNetFd, &stFdSet);
	iRet = select (FD_SETSIZE, &stFdSet, NULL, NULL, &stTime);
	if(0 == iRet)
	{
		tl_controler_timeout(iAddress);
		return ERROR;
	}
	if(OK !=  tl_com_read_packet(pCom));
		return ERROR;
	while(TL_COM_PKT_WAIT == pCom->iLineSate)
	{
		stTime.tv_sec = pGlobal->iTimeout / 1000;
		stTime.tv_usec = 1000*(pGlobal->iTimeout % 1000);
		FD_ZERO(&stFdSet);		
		FD_SET (pGlobal->stPort.iNetFd, &stFdSet);
		iRet = select (FD_SETSIZE, &stFdSet, NULL, NULL, &stTime);
		if(0 == iRet)
		{
			tl_controler_timeout(iAddress);
			return ERROR;
		}
		if(OK !=  tl_com_read_packet(pCom));
			return ERROR;	
	}
	return OK;
}

int tl_com_port_open(tl_com_t *pCom)
{
	struct termios stOptions;
	tl_global_t *pGlobal = tl_get_global();
	
	pCom->iNetFd = open(pCom->szPortName, O_RDWR | O_NOCTTY | O_NDELAY);
	if(-1 == pCom->iNetFd)
	{
		tl_log_error("Failed to open com %s chanel, errno %d (%s)", 
			pCom->szPortName, errno, strerror(errno));
		return ERROR;
	}
	pCom->iRcvSize = 0;
	pCom->arrRecived[0]=0;

	if(-1 == fcntl(pCom->iNetFd, F_SETFL, FNDELAY))
	{
		tl_log_error("Failed to set com %s to NDELAY, errno %d (%s)", 
			pCom->szPortName, errno, strerror(errno));
		return ERROR;
	}
	
	tcgetattr(pCom->iNetFd, &stOptions);
	cfsetispeed(&stOptions, pCom->iPortSpeed);
    cfsetospeed(&stOptions, pCom->iPortSpeed);
	
	stOptions.c_cflag &= ~CSIZE;
	stOptions.c_cflag &= ~CSTOPB;
	stOptions.c_cflag &= ~PARENB;
	
	stOptions.c_cflag |= (CLOCAL | CREAD);
	stOptions.c_cflag |= pGlobal->stPort.iPortFlags;

	stOptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	if(-1 == tcsetattr(pCom->iNetFd, TCSAFLUSH, &stOptions))
	{
		tl_log_error("Failed to set com %s attributes, errno %d (%s)", 
			pCom->szPortName, errno, strerror(errno));
		return ERROR;
	}

	
	pCom->pReadThread  = thread_read_add(&(pGlobal->stMaintThread), tl_com_read, pCom, pCom->iNetFd);
	
	if(TL_IS_DEBUG(INIT))
		tl_log_info("Com port %s is ready", pCom->szPortName);
	
	return OK;
}

