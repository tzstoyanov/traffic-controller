#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_packets_api.h"
#include "tl_program_api.h"
#include "tl_serial_api.h"
#include "lib_tl_log.h"

tl_log_dest_t iTlLogDestination = TL_LOG2CONSOLE;
uint32_t iTlightDebugSwitch = 0;

int tl_log_init(tl_log_dest_t  iDest, char *szLogPrefix)
{
	openlog(szLogPrefix, LOG_CONS, LOG_LOCAL3);
	return OK;
}

void tl_log_dest_set(tl_log_dest_t  iDest)
{
	iTlLogDestination = iDest;
}

void tl_log_info(const char *format, ... )
{
	va_list args;
	
	if(TL_LOG2NULL == iTlLogDestination)
		return;
	
	va_start(args, format);
	
	if(TL_LOG2CONSOLE == iTlLogDestination)
	{
		vprintf(format, args);
		printf("%s", TL_LOG_NEWLINE);
		fflush(stdout);
	}
	else
		vsyslog(LOG_DEBUG, format, args);
	
	va_end(args);
}

void tl_log_error(const char *format, ... )
{
	va_list args;

	if(TL_LOG2NULL == iTlLogDestination)
		return;
	
	va_start(args, format);
	
	if(TL_LOG2CONSOLE == iTlLogDestination)
	{
		vprintf(format, args);
		printf("%s", TL_LOG_NEWLINE);
		fflush(stdout);		
	}
	else
		vsyslog(LOG_ERR, format, args);
	
	va_end(args);
}

char *tl_status2string(uint8_t iStatus)
{
	switch(iStatus)
	{
		case TL_MODULE_ERR_OK:			return "OK";
		case TL_MODULE_ERR_INIT:		return "Init";
		case TL_MODULE_ERR_HW_ERR:		return "Critical Hardware failure";
		case TL_MODULE_ERR_HW_WARN:		return "Warning Hardware failure";
		case TL_MODULE_ERR_COMM:		return "Communication error";
		case TL_MODULE_ERR_SLAVE:		return "Slave error";
		case TL_MODULE_ERR_TIMEOUT:		return "Communication timeout";
		case TL_MODULE_ERR_MISMATCH:	return "Mismatch";
		default: break;
	}
	return "Uknown";
}

char *tl_type2string(uint8_t iType)
{
	switch(iType)
	{
		case TL_MODULE_MASTER:	return "Master";
		case TL_MODULE_GATE:	return "Gateway";
		case TL_MODULE_LAMP:	return "Lamp";
		case TL_MODULE_TIMER:	return "Timer";
		default: break;
	}
	return "Uknown";
}

char *tl_color2string(uint8_t iColor)
{
	switch(iColor)
	{
		case TL_TIMER_COLOR_GREEN: 	return "Green";
		case TL_TIMER_COLOR_RED: 	return "Red";
		default: break;
	}
	return "Uknown";
}

char *tl_serialstate2string(tl_com_state_t iState)
{
	switch(iState)
	{	
		case TL_COM_IDLE: 				return "Idle";
		case TL_COM_LOCAL_VERIFY:		return "Verify";
		case TL_COM_LOCAL_VERIFY_WAIT:	return "Verify Wait";	
		case TL_COM_PKT_WAIT:			return "Wait";
		default: break;		
	}	
	return "Uknown";
}

char *tl_programstate2string(tl_program_state_t iState)
{
	switch(iState)
	{
		case TL_PROGRAM_RUNNING:	return "Running";
		case TL_PROGRAM_PAUSE:		return "Pause";
		case TL_PROGRAM_EXITING:	return "Exiting";
		case TL_PROGRAM_EXIT:		return "Exit";
		default: break;
	}
 return "Uknown";	
}

char *tl_command2string(uint8_t iCommand)
{
	switch(iCommand)
	{
		case TL_CMD_CONFIG:			return "Config";
		case TL_CMD_STATE:			return "State";
		case TL_CMD_WD_REQUEST:		return "Request";
		case TL_CMD_WD_RESPONCE:	return "Responce";
		case TL_CMD_WD_RESPONCE_F:	return "Responce fragment";
		default: break;
	}
	return "Uknown";
}

char *tl_lampstate2string(tl_lamp_state_t *pState)
{	
	static char szState[30];
	snprintf(szState, 30, "%s %s %s", 
			(pState->iState & 0x04)?"Red":"", 
			(pState->iState & 0x02)?"Yellow":"",
			(pState->iState & 0x01)?"Green":"");
	szState[29]=0;
	return szState;
}

char *tl_timerstate2string(tl_timer_state_t *pState)
{
	static char szState[50];
	
	if(TL_TIMER_ACT_STATIC == pState->iAction)
		snprintf(szState, 50, "Display Static 0x%02X 0x%02X in color %s",
				pState->arrState[0], pState->arrState[1], tl_color2string(pState->iColor));	
	else if(TL_TIMER_ACT_COUNTDOWN )	
		snprintf(szState, 50, "Countown %ds in color %s",
				pState->arrState[0], tl_color2string(pState->iColor));	
	else
		snprintf(szState, 50, "Uknown Action %d", pState->iAction );
	szState[49]=0;
	return szState;	
}

void tl_debug_switch_set(uint32_t iDebug)
{
	 iTlightDebugSwitch = iDebug;
	 tl_log_info("Set debug flag to 0x%X", iDebug);
}

int tl_debug_get(uint32_t *pDebugSwitch, uint32_t *pLogDest)
{
	*pDebugSwitch = iTlightDebugSwitch;
	*pLogDest = iTlLogDestination;
	return OK;
}

