#include "os_linux.h"
#include "tl_types.h"
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


