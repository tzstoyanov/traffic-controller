#ifndef __TLIGHT_MAIN_H__
#define __TLIGHT_MAIN_H__

#define TL_LOG_PREFIX	"TL"

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
	int iApiFd;
	thread_t	*pApiThread;
	thread_t	*arrClients[TL_API_MAX_CONNECTIONS];
}tl_global_t;

tl_global_t *tl_get_global();

void tl_debug_switch_set(uint32_t iDebug);
int tl_debug_get(uint32_t *pDebugSwitch, uint32_t *pLogDest);

extern uint32_t iTlightDebugSwitch;
#define TL_IS_DEBUG(x) (iTlightDebugSwitch & TL_DEBUG_ ## x)

#endif /* __TLIGHT_MAIN_H__ */


