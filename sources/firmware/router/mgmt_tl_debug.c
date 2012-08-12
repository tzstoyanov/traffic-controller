#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_api.h"
#include "lib_tl_log.h"
#include "mgmt_tl_main.h"

void print_help_debug()
{
	tl_log_info("\t--debug help\tShow this debug help");	
	tl_log_info("\t--debug flag=HEX\tSet Debug flags, HEX is bitmask of:");
	tl_log_info("\t\t %08X\tGateway related logs", TL_DEBUG_GATE);
	tl_log_info("\t\t %08X\tLamp related logs", TL_DEBUG_LAMP);	
	tl_log_info("\t\t %08X\tTimer related logs", TL_DEBUG_TIMER);	
	tl_log_info("\t\t %08X\tInit process", TL_DEBUG_INIT);			
	tl_log_info("\t\t %08X\tPackets receive", TL_DEBUG_RX);	
	tl_log_info("\t\t %08X\tCfg Packets send", TL_DEBUG_TX_CFG);	
	tl_log_info("\t\t %08X\tCmd Packets send", TL_DEBUG_TX_CMD);	
	tl_log_info("\t\t %08X\tRequest Packets send", TL_DEBUG_TX_RQST);		
	tl_log_info("\t\t %08X\tDump Packets contents", TL_DEBUG_PKT_DUMP);	
	tl_log_info("\t\t %08X\tConfig files parse", TL_DEBUG_CONFIG);	
	tl_log_info("\t\t %08X\tManagement requests", TL_DEBUG_API);	
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


