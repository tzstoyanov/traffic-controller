#ifndef __TLIGHT_SIM_MAIN_H__
#define __TLIGHT_SIM_MAIN_H__

#define TL_SIM_LOG_PREFIX	"TLSIM"

#define TL_SIM_MAX_DUT	3

typedef struct
{
	thread_t	*pWDThread;	
	main_thread_t stMaintThread;
	int iApiFd;
	int iLampCount;
	int iTimerCount;
	int iGateCount;
	uint8_t iUtest;
	uint32_t iTestStepDelay;
	uint32_t iTestStepCount;
	uint32_t iTestInProgress;
	thread_t *pUTThread;	
	uint8_t arrDut[TL_SIM_MAX_DUT];
}tl_sim_global_t;

typedef struct
{
	int32_t iWdInterval;
	int32_t iDeadCount;
	int32_t iComTimeout;
	uint8_t arrSlaves[TL_MAX_CONTROLERS];
	uint8_t iSlavesAttached;
	struct timeval stLastStateTime;	
	uint32_t arrCfgReceived[TL_CFGID_SLAVES];
	uint8_t iFailSafeState;
	int iSimStatus;
	uint8_t iAdaptiveStateInit;
	int iSimSegments;
	uint8_t iSimStatusBlock;
}tl_sim_controler_t;

tl_sim_global_t *tl_get_global_sim();
int tl_reply_status(tl_controler_t *pControler, t_can_packet_t *pPacket);
int32_t sim_tl_com_read(thread_t *pThread);
tl_controler_t *sim_tl_NewControler(uint8_t iType);
int sim_tl_controler(uint8_t iAddress, t_can_packet_t *pPacket);
void sim_get_time_rel(struct timeval *pNow);
int sim_get_client_fd();
int sim_tl_utest_init();

int sim_tl_check_status(uint8_t iDevError, uint8_t iCritical, 
						uint8_t iWarning, tl_program_state_t iProgState);

#endif /* __TLIGHT_SIM_MAIN_H__ */
