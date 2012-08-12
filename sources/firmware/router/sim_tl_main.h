#ifndef __TLIGHT_SIM_MAIN_H__
#define __TLIGHT_SIM_MAIN_H__

#define TL_SIM_LOG_PREFIX	"TLSIM"

typedef struct
{
	thread_t	*pWDThread;	
	main_thread_t stMaintThread;
}tl_sim_global_t;

typedef struct
{
	int32_t iWdInterval;
	int32_t iDeadCount;
	uint8_t arrSlaves[TL_MAX_CONTROLERS];
}tl_sim_controler_t;

int tl_reply_status(tl_controler_t *pControler, t_can_packet_t *pPacket);
int32_t sim_tl_com_read(thread_t *pThread);
tl_controler_t *sim_tl_NewControler(uint8_t iType);
int sim_tl_controler(uint8_t iAddress, t_can_packet_t *pPacket);

#endif /* __TLIGHT_SIM_MAIN_H__ */
