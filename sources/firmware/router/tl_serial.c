#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_api.h"
#include "tl_serial.h"
#include "tl_program.h"
#include "tl_packets_api.h"
#include "lib_tl_packets.h"
#include "tl_controlers.h"
#include "lib_tl_log.h"
#include "lib_tl_config.h"
#include "lib_tl_com.h"
#include "tl_main.h"

int32_t tl_com_read(thread_t *pThread)
{
	tl_com_t *pCom = (tl_com_t*) pThread->pArg;
	
	pCom->pReadThread  = NULL;
	
	if(TL_IS_DEBUG(RX))
		tl_log_info("Recived unexpected data from %s, line state is %d", 
				pCom->szPortName, pCom->iLineSate );
				
	tl_com_read_packet(pCom);
	
	pCom->pReadThread  = thread_read_add(pThread->pMain, tl_com_read, pCom, pCom->iNetFd);
	return 1;
}

int tl_com_send_session(tl_com_t *pCom, uint8_t iAddress, uint8_t iNextState)
{

	if(OK != tl_com_send(pCom))
		return ERROR;

	pCom->iLineSate = iNextState;
	while(TL_COM_IDLE != pCom->iLineSate)
	{
		if(OK != tl_com_read_packet(pCom))
		{
			tl_controler_timeout(iAddress);
			pCom->iLineSate = TL_COM_IDLE;
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
	stOptions.c_cflag |= pCom->iPortFlags;

	stOptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	if(-1 == tcsetattr(pCom->iNetFd, TCSAFLUSH, &stOptions))
	{
		tl_log_error("Failed to set com %s attributes, errno %d (%s)", 
			pCom->szPortName, errno, strerror(errno));
		return ERROR;
	}

	pCom->iLineSate = TL_COM_IDLE;

	if(TL_IS_DEBUG(INIT))
		tl_log_info("Com port %s is ready", pCom->szPortName);
	
	return OK;
}

int tl_com_port_open(tl_com_t *pCom)
{
	if(TL_COM_SERIAL == pCom->iType)
		return tl_com_port_serial_open(pCom);
	return 	tl_com_sym_open(pCom);
}

