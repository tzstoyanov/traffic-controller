#ifndef __TLIGHT_SERIAL_H__
#define __TLIGHT_SERIAL_H__

typedef enum
{
	TL_COM_IDLE,
	TL_COM_LOCAL_VERIFY,
	TL_COM_LOCAL_VERIFY_WAIT,	
	TL_COM_PKT_WAIT
}tl_com_state_t;

typedef struct
{
	char *szPortName;
	uint32_t iPortSpeed;	
	uint32_t iPortFlags;	
	int	iNetFd;
	tl_com_state_t iLineSate;
	char arrSend[TL_MAX_PACKET_SERIAL_SIZE];
	uint8_t	iSendSize;
	char arrRecived[TL_MAX_PACKET_SERIAL_SIZE];
	uint8_t	iRcvSize;
	thread_t *pReadThread;
}tl_com_t;

int tl_com_port_open(tl_com_t *pCom);
int tl_com_send_session(tl_com_t *pCom, uint8_t iAddress, uint8_t iNextState);

#endif /* __TLIGHT_SERIAL_H__ */

