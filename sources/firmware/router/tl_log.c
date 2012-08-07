#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_api.h"
#include "tl_serial.h"
#include "tl_program.h"
#include "tl_packets.h"
#include "tl_controlers.h"
#include "tl_config.h"
#include "tl_log.h"

static tl_log_dest_t iTlLogDestination = TL_LOG2CONSOLE;
uint32_t iTlightDebugSwitch = 0;


int tl_log_init(tl_log_dest_t  iDest)
{
	openlog(TL_LOG_PREFIX, LOG_CONS, LOG_LOCAL3);
	return OK;
}

void tl_log_dest_set(tl_log_dest_t  iDest)
{
	iTlLogDestination = iDest;
}

void tl_debug_switch_set(uint32_t iDebug)
{
	 iTlightDebugSwitch = iDebug;
	 tl_log_info("Set debug flag to 0x%X", iDebug);
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


