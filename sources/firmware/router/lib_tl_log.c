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
	iTlLogDestination = iDest;
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
		putchar('\n');
		putchar('\r');
		fflush(stdout);
	}
	else
		vsyslog(LOG_DEBUG, format, args);
	
	va_end(args);
}

void tl_log_info_nonr(const char *format, ... )
{
	va_list args;
	
	if(TL_LOG2NULL == iTlLogDestination)
		return;
	
	va_start(args, format);
	
	if(TL_LOG2CONSOLE == iTlLogDestination)
	{
		vprintf(format, args);
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
		putchar('\n');
		putchar('\r');
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
		case TL_MODULE_STATE_OK:		return "OK";
		case TL_MODULE_STATE_INIT:		return "Init";
		case TL_MODULE_STATE_HW_ERR:	return "Critical Hardware failure";
		case TL_MODULE_STATE_HW_WARN:	return "Warning Hardware failure";
		case TL_MODULE_STATE_COMM_ERR:	return "Communication error";
		case TL_MODULE_STATE_TIMEOUT:	return "Communication timeout";
		case TL_MODULE_STATE_MISMATCH:	return "Mismatch";
		case TL_MODULE_STATE_FAILSAFE:	return "FailSafe";
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
		case TL_COM_CONFLICT: 			return "Conflict";
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

int tl_clock_gettime(struct timeval *pNow)
{
	return gettimeofday(pNow,NULL);
}

void tl_print_help_debug_flag()
{	
	tl_log_info("\t\t %08X\tGateway related logs", TL_DEBUG_GATE);
	tl_log_info("\t\t %08X\tLamp related logs", TL_DEBUG_LAMP);	
	tl_log_info("\t\t %08X\tTimer related logs", TL_DEBUG_TIMER);	
	tl_log_info("\t\t %08X\tInit process", TL_DEBUG_INIT);			
	tl_log_info("\t\t %08X\tPackets receive", TL_DEBUG_RX);	
	tl_log_info("\t\t %08X\tCfg Packets send", TL_DEBUG_TX_CFG);	
	tl_log_info("\t\t %08X\tCmd Packets send", TL_DEBUG_TX_CMD);	
	tl_log_info("\t\t %08X\tRequest Packets send", TL_DEBUG_TX_RQST);		
	tl_log_info("\t\t %08X\tDump Packets contents", TL_DEBUG_PKT_DUMP);
	tl_log_info("\t\t %08X\tProgram execution", TL_DEBUG_PROGRAM);	
	tl_log_info("\t\t %08X\tConfig files parse", TL_DEBUG_CONFIG);	
	tl_log_info("\t\t %08X\tManagement requests", TL_DEBUG_API);
}

void tl_print_help_debug_dest()
{
	tl_log_info("\t\t Null\t\tDo not log");
	tl_log_info("\t\t Console\tLog to console, default");
	tl_log_info("\t\t Syslog\t\tLog to syslog");
}


void get_time_rel(struct timeval *pBegin, struct timeval *pNow)
{
	tl_clock_gettime(pNow);
	pNow->tv_sec -= pBegin->tv_sec;
	if(pNow->tv_usec >= pBegin->tv_usec)
		pNow->tv_usec -= pBegin->tv_usec;
	else
		pNow->tv_usec += (1000000 - pBegin->tv_usec);	
}

