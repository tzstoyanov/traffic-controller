#ifndef __TLIGHT_SYM_MAIN_H__
#define __TLIGHT_SYM_MAIN_H__

#define TL_SYM_LOG_PREFIX	"TL"

typedef struct
{
	thread_t	*pWDThread;	
	main_thread_t stMaintThread;
}tl_sym_global_t;

#endif /* __TLIGHT_SYM_MAIN_H__ */
