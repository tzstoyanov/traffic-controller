#ifndef __TLIGHT_MAIN_H__
#define __TLIGHT_MAIN_H__

#define TL_LOG_PREFIX	"TL"

void tl_check_system_status();

typedef struct
{
	uint32_t iCritError;
	uint32_t iWarning;
	tl_program_t *pCurrentProgram;
	main_thread_t stMaintThread;
	thread_t	*pWDThread;	
	int iApiFd;
	thread_t	*pApiThread;
	thread_t	*arrClients[TL_API_MAX_CONNECTIONS];
}tl_global_t;

tl_global_t *tl_get_global();

void tl_exit_callback(void *pCookie);

#endif /* __TLIGHT_MAIN_H__ */


