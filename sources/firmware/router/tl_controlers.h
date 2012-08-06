#ifndef __TLIGHT_DBASE_H__
#define __TLIGHT_DBASE_H__

typedef int (*tl_controler_check_callback_t)(uint8_t iAddress, t_can_packet_t *pPacket);	

typedef struct
{
	uint8_t	iId;
	uint8_t iType;
	char *szName;
	uint8_t iGateId;
	uint8_t	iStatus;
	tl_controler_check_callback_t fVerifyState;
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

typedef struct
{
	char *szCrossName;
	tl_com_t stPort;
	int32_t iWdInterval;
	int32_t	iTimeout;
	tl_controler_t *arrDevices[TL_MAX_CONTROLERS];
	tl_program_t *pCurrent;
	main_thread_t stMaintThread;
	thread_t	*pWDThread;	
	thread_t	*pTimeOut;
}tl_global_t;

tl_global_t *tl_get_global();
tl_controler_t *tl_NewControler(uint8_t iType);
tl_controler_t *tl_get_controler_byname(char *szName);
int tl_check_controlers();
int tl_ask_controler_state(uint8_t iAddress);
int tl_update_controler_state(tl_instruction_t *pCommand);
int tl_controler_timeout(uint8_t iAddress);

#endif /* __TLIGHT_DBASE_H__ */
