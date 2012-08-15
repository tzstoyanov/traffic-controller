#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_packets_api.h"
#include "tl_api.h"
#include "tl_serial.h"
#include "tl_program.h"
#include "tl_controlers.h"
#include "lib_tl_log.h"
#include "lib_tl_config.h"
#include "lib_tl_com.h"
#include "lib_tl_packets.h"
#include "tl_main.h"

int tl_com_pkt_handle(tl_com_t *pCom)
{
	t_can_packet_t stPacket;
	tl_global_config_t *pGlobal = tl_get_global_dbase();

	pCom->iLineState = TL_COM_IDLE;
	if(OK == tl_command_extract(pCom->arrRecived, pCom->iRcvSize, &stPacket))
	{
		if(	TL_ADDRESS_BCAST == stPacket.iAddress || 
			(NULL != pGlobal->arrDevices[stPacket.iAddress] && 
			TL_MODULE_MASTER == pGlobal->arrDevices[stPacket.iAddress]->iType) )
		{
			if(TL_IS_DEBUG(RX))
				tl_log_info("Recived packet %s from %d", 
					tl_command2string(stPacket.iCommand), stPacket.arrData[0]);
			if(TL_CMD_WD_RESPONCE_F == stPacket.iCommand)
				pCom->iLineState = TL_COM_PKT_WAIT;
	 		if(	NULL != pGlobal->arrDevices[stPacket.arrData[0]] && 
				NULL != pGlobal->arrDevices[stPacket.arrData[0]]->fPktProcess)
				pGlobal->arrDevices[stPacket.arrData[0]]->fPktProcess(stPacket.arrData[0], &stPacket);
		}
	}
	return OK;
}

int tl_com_local_pkt_verify(tl_com_t *pCom)
{
	if(pCom->iRcvSize != pCom->iSendSize || 0 != memcmp(pCom->arrSend, pCom->arrRecived, pCom->iRcvSize))
	{
		tl_log_error("Local Echo check failed !");
		pCom->iLineErrors++;
		pCom->iLineState = TL_COM_CONFLICT;
		return ERROR;
	}
	if(TL_COM_LOCAL_VERIFY_WAIT == pCom->iLineState)
		pCom->iLineState = TL_COM_PKT_WAIT;
	else
		pCom->iLineState = TL_COM_IDLE;
	return OK;
}

int tl_com_pakcet_process(tl_com_t *pCom)
{
 int iRet;
 	if(pCom->iRcvSize < 1)
 		return ERROR;	
	if(TL_PKT_END == pCom->arrRecived[pCom->iRcvSize-1])
	{	
		if(TL_IS_DEBUG(RX))
			tl_log_info("Line state is %s", tl_serialstate2string(pCom->iLineState) );
	
		switch(pCom->iLineState)
		{
			case TL_COM_LOCAL_VERIFY:
			case TL_COM_LOCAL_VERIFY_WAIT:
				tl_com_local_pkt_verify(pCom);
				break;
			case TL_COM_IDLE:
			case TL_COM_PKT_WAIT:
				tl_com_pkt_handle(pCom);
				break;
			default:
				break;
			iRet = OK;	
		}
		iRet = OK;
	}
	else
	{
		pCom->iLineState = TL_COM_IDLE;
		iRet = ERROR;
	}
	return iRet;
}

void tl_com_flush(tl_com_t *pCom, uint32_t iTimeout)
{
	int iRet;
	fd_set stFdSet;
	char iBuf;
	struct timeval stTime;
	do
	{
		stTime.tv_sec = pCom->iTimeout / 1000;
		stTime.tv_usec = 1000*(pCom->iTimeout % 1000);
		FD_ZERO(&stFdSet);		
		FD_SET (pCom->iNetFd, &stFdSet);
		iRet = select (FD_SETSIZE, &stFdSet, NULL, NULL, &stTime);
		if(iRet <= 0)
			return;
		if( 1 != read(pCom->iNetFd, &iBuf, 1))
			return;	
	}while(1);	
}	

int tl_com_read_master(tl_com_t *pCom)
{
	int iRet;
	
	iRet = tl_com_read_packet(pCom);
	if(OK == iRet)
		iRet = tl_com_pakcet_process(pCom);
	if(TL_COM_CONFLICT == pCom->iLineState)
	{
		tl_com_flush(pCom, 3*(pCom->iTimeout));
		pCom->iLineState = TL_COM_IDLE;
	}
	pCom->iRcvSize = 0;
	pCom->arrRecived[0]=0;

	return iRet;
}


int32_t tl_com_read(thread_t *pThread)
{
	tl_com_t *pCom = (tl_com_t*) pThread->pArg;
	
	pCom->pReadThread  = NULL;
	
	if(TL_IS_DEBUG(RX))
		tl_log_info("Recived unexpected data from %s, line state is %d", 
				pCom->szPortName, pCom->iLineState );
				
	tl_com_read_master(pCom);
	
	if(pCom->iNetFd >= 0)
		pCom->pReadThread  = thread_read_add(pThread->pMain, tl_com_read, pCom, pCom->iNetFd);
	else
		tl_log_error("Communincation socket closed");
	return 1;
}

int32_t tl_com_accept(thread_t *pThread)
{
	int iAccFd;
	tl_com_t *pCom = (tl_com_t*) pThread->pArg;
	
	pCom->pAcceptThread  = NULL;
	
	iAccFd = accept(pCom->iAccFd, NULL, 0);
	if(-1 != pCom->iNetFd)
	{
		tl_log_error("Cannot accept new connection");	
		close(iAccFd);
	}				
	else
	{
		pCom->iNetFd = iAccFd;
		tl_log_info("New local connection accepted");
	}
	pCom->pAcceptThread  = thread_read_add(pThread->pMain, tl_com_accept, pCom, pCom->iAccFd);
	return 1;
}

int tl_com_send_session(tl_com_t *pCom, uint8_t iAddress, uint8_t iNextState)
{

	if(OK != tl_com_send(pCom))
	{
		tl_controler_timeout(iAddress);
		pCom->iLineState = TL_COM_IDLE;	
		return ERROR;
	}
	pCom->iLineState = iNextState;
	while(TL_COM_IDLE != pCom->iLineState)
	{
		if(OK != tl_com_read_master(pCom))
		{
			tl_controler_timeout(iAddress);
			pCom->iLineState = TL_COM_IDLE;
			return ERROR;	
		}	
	}

	return OK;
}

int tl_com_port_serial_open(tl_com_t *pCom)
{
	struct termios stOptions;
	
	pCom->iNetFd = open(pCom->szPortName, O_RDWR | O_NOCTTY | O_NDELAY);
	if(-1 == pCom->iNetFd)
	{
		tl_log_error("Failed to open com %s channel, errno %d (%s)", 
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
	stOptions.c_cflag |= pCom->iPortFlags;

	stOptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	if(-1 == tcsetattr(pCom->iNetFd, TCSAFLUSH, &stOptions))
	{
		tl_log_error("Failed to set com %s attributes, errno %d (%s)", 
			pCom->szPortName, errno, strerror(errno));
		return ERROR;
	}

	pCom->iLineState = TL_COM_IDLE;

	if(TL_IS_DEBUG(INIT))
		tl_log_info("Com port %s is ready", pCom->szPortName);
	
	return OK;
}

int tl_com_port_open(tl_com_t *pCom)
{
	if(TL_COM_SERIAL == pCom->iType)
		return tl_com_port_serial_open(pCom);
	return 	tl_com_sim_open(pCom);
}

