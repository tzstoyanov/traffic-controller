#include "os_linux.h"
#include "tl_types.h"
#include "tl_program_api.h"
#include "tl_packets_api.h"
#include "tl_log.h"

tl_log_dest_t iTlLogDestination = TL_LOG2CONSOLE;

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
		case TL_MODULE_ERR_HW:			return "Hardware failure";
		case TL_MODULE_ERR_COMM:		return "Communication error";
		case TL_MODULE_ERR_SLAVE:		return "Slave error";
		case TL_MODULE_ERR_TIMEOUT:		return "Communication timeout";
		case TL_MODULE_ERR_MISMATCH:	return "State mismatch";
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
		case TL_TIMER_COLOR_GREEN: return "Green";
		case TL_TIMER_COLOR_RED: return "Red";
		default: break;
	}
	return "Uknown";
}

char *tl_prrogramstate2string(tl_program_state_t iState)
{
	switch(iState)
	{
		case TL_PROGRAM_RUNNING:	return "Running";
		case TL_PROGRAM_PAUSE:		return "Pause";
		default: break;
	}
 return "Uknown";	
}

