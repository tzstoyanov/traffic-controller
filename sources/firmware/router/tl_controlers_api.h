#ifndef __TLIGHT_CONTROLERS_API_H__
#define __TLIGHT_CONTROLERS_API_H__

typedef int (*tl_controler_packet_callback_t)(uint8_t iAddress, t_can_packet_t *pPacket);	

typedef struct
{
	uint8_t	iId;
	uint8_t iType;
	char *szName;
	uint8_t iSendConfig;
	uint8_t iGateId;
	uint8_t	iStatus;
	uint8_t iCritical;
	tl_controler_packet_callback_t fPktProcess;
}tl_controler_t;

typedef struct
{
	tl_controler_t stHead;
	tl_lamp_state_t stCurrentState;
}tl_controler_lamp_t;

typedef struct
{
	tl_controler_t stHead;
	tl_timer_state_t stCurrentState;
}tl_controler_timer_t;

typedef tl_controler_t* (tl_new_controler_callback_t)(uint8_t iType);

#endif /* __TLIGHT_CONTROLERS_API_H__ */

