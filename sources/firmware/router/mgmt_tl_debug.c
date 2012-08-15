#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_api.h"
#include "lib_tl_log.h"
#include "lib_tl_client.h"
#include "mgmt_tl_main.h"

void print_help_debug()
{
	tl_log_info("\t--debug help\tShow this debug help");	
	tl_log_info("\t--debug flag=HEX\tSet Debug flags, HEX is bitmask of:");
	tl_print_help_debug();
	tl_log_info("\t--debug dest=NAME\tSet Debug destination, where NAME can be:");
	tl_log_info("\t\tNULL\t\tDo not log");
	tl_log_info("\t\tConsole\t\tLog to console");
	tl_log_info("\t\tSyslog\t\tLog to Syslog");
}

void process_debug(char *szArgument, char *szValue)
{
	uint8_t iCommand, *pData = NULL, iDest, iLen, iFail=0;
	uint32_t iFlags;
	
	if(0 == strncmp(szArgument, "flag", strlen("flag")))
	{
		iCommand = TL_API_SET_DEBUG_FLAG;
		iLen = 4;
		pData = (uint8_t*)&iFlags;
		iFlags = strtol(szValue, NULL, 16);
	}else if(0 == strncmp(szArgument, "help", strlen("help")))	
	{
		print_help_debug();
		fflush(stdout);
		return;
	}else if(0 == strncmp(szArgument, "dest", strlen("dest")))
	{
		iCommand = TL_API_SET_DEBUG_DEST;
		iLen = 4;
		pData = &iDest;
		if(0 == strncmp(szValue, "NULL", strlen("NULL")))
		{
			iDest = TL_LOG2NULL;
		}else if(0 == strncmp(szValue, "Console", strlen("Console")))
		{
			iDest = TL_LOG2CONSOLE;
		}else if(0 == strncmp(szValue, "Syslog", strlen("Syslog")))
		{
			iDest = TL_LOG2SYSLOG;
		}else
			iFail++;
	}else
		iFail++;
	
	if(0 == iFail)
	{
		tl_mgmt_send(get_client_fd(), iCommand, pData, iLen);
		return;
	}
	
	tl_log_info("Uknown debug arguments %s,%s, Usage:", szArgument, szValue);
	print_help_debug();
	fflush(stdout);
}


