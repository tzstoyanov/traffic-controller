#ifndef __TLIGHT_SERIAL_API_H__
#define __TLIGHT_SERIAL_API_H__

typedef enum
{
	TL_COM_IDLE = 0,
	TL_COM_CONFLICT,
	TL_COM_LOCAL_VERIFY,
	TL_COM_LOCAL_VERIFY_WAIT,	
	TL_COM_PKT_WAIT
}tl_com_state_t;

typedef enum
{
	TL_COM_SERIAL = 0,
	TL_COM_LOCALSOCK
}tl_com_type_t;

typedef struct
{
	char *szPortName;
	uint32_t iPortSpeed;	
	uint32_t iPortFlags;	
	tl_com_type_t iType;
	uint8_t	iTimeout;
	int	iNetFd;
	int iAccFd;
	tl_com_state_t iLineState;
	char arrSend[TL_MAX_PACKET_SERIAL_SIZE];
	uint8_t	iSendSize;
	char arrRecived[TL_MAX_PACKET_SERIAL_SIZE];
	uint8_t	iRcvSize;
	thread_t *pReadThread;
	thread_t *pAcceptThread;
	uint32_t iLineErrors;
}tl_com_t;

#endif /*  __TLIGHT_SERIAL_API_H__ */
