#ifndef __TLIGHT_LOG_H__
#define __TLIGHT_LOG_H__

#define TL_LOG_PREFIX	"TL"

typedef enum
{
	TL_LOG2NULL = 0,
	TL_LOG2CONSOLE = 1,
	TL_LOG2SYSLOG = 2
}tl_log_dest_t;

extern uint32_t iTlightDebugSwitch;

#define TL_DEBUG_DEVICES	0x000F
#define TL_DEBUG_GATE		0x0001		
#define TL_DEBUG_LAMP		0x0002
#define TL_DEBUG_TIMER		0x0004
#define TL_DEBUG_INIT		0x0010
#define TL_DEBUG_RX			0x0020
#define TL_DEBUG_TX			0x01C0
#define TL_DEBUG_TX_CFG		0x0040
#define TL_DEBUG_TX_CMD		0x0080
#define TL_DEBUG_TX_RQST	0x0100
#define TL_DEBUG_PKT_DUMP	0x0200
#define TL_DEBUG_PROGRAM	0x0400
#define TL_DEBUG_CONFIG		0x0800
#define TL_DEBUG_API		0x1000

#define TL_IS_DEBUG(x) (iTlightDebugSwitch & TL_DEBUG_ ## x)

#define TL_LOG_NEWLINE	"\n"

int tl_log_init(tl_log_dest_t  iDest);
void tl_log_dest_set(tl_log_dest_t  iDest);
void tl_debug_switch_set(uint32_t iDebug);

void tl_log_info(const char *format, ... );
void tl_log_error(const char *format, ... );

#endif /* __TLIGHT_LOG_H__ */
