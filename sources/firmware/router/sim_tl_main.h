#ifndef __TLIGHT_SIM_MAIN_H__
#define __TLIGHT_SIM_MAIN_H__

#define TL_SIM_LOG_PREFIX	"TLSIM"

typedef struct
{
	thread_t	*pWDThread;	
	main_thread_t stMaintThread;
	int iApiFd;
	uint8_t iUtest;
	uint32_t iTestStepDelay;
	uint32_t iTestStepCount;
	uint32_t iTestInProgress;
	thread_t	*pUTThread;	
}tl_sim_global_t;

typedef struct
{
	
}tl_utest_context_t;

typedef struct
{
	int32_t iWdInterval;
	int32_t iDeadCount;
	uint8_t arrSlaves[TL_MAX_CONTROLERS];
	struct timeval stLastStateTime;	
	tl_utest_context_t stUtest;
}tl_sim_controler_t;

tl_sim_global_t *tl_get_global_sim();
int tl_reply_status(tl_controler_t *pControler, t_can_packet_t *pPacket);
int32_t sim_tl_com_read(thread_t *pThread);
tl_controler_t *sim_tl_NewControler(uint8_t iType);
int sim_tl_controler(uint8_t iAddress, t_can_packet_t *pPacket);
void sim_get_time_rel(struct timeval *pNow);
int sim_get_client_fd();
int sim_tl_utest_init();

#endif /* __TLIGHT_SIM_MAIN_H__ */
