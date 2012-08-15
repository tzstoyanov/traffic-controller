#ifndef __TLIGHT_LOG_H__
#define __TLIGHT_LOG_H__

#include "tl_program_api.h"
#include "tl_serial_api.h"

typedef enum
{
	TL_LOG2NULL = 0,
	TL_LOG2CONSOLE = 1,
	TL_LOG2SYSLOG = 2
}tl_log_dest_t;

extern uint32_t iTlightDebugSwitch;
extern tl_log_dest_t iTlLogDestination;

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

int tl_log_init(tl_log_dest_t  iDest, char *szPrefix);
void tl_log_dest_set(tl_log_dest_t  iDest);

void tl_log_info(const char *format, ... );
void tl_log_error(const char *format, ... );

int tl_clock_gettime(struct timeval *pNow);

char *tl_status2string(uint8_t iStatus);
char *tl_type2string(uint8_t iType);
char *tl_color2string(uint8_t iColor);
char *tl_programstate2string(tl_program_state_t iState);
char *tl_lampstate2string(tl_lamp_state_t *pState);
char *tl_timerstate2string(tl_timer_state_t *pState);

void tl_debug_switch_set(uint32_t iDebug);
int tl_debug_get(uint32_t *pDebugSwitch, uint32_t *pLogDest);
char *tl_serialstate2string(tl_com_state_t iState);
char *tl_command2string(uint8_t iCommand);

void tl_print_help_debug();

#endif /* __TLIGHT_LOG_H__ */
