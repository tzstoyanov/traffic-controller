#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_api.h"
#include "tl_log.h"
#include "tl_mgmt_client.h"

int iClientFd=-1;

void print_help_debug()
{
	printf("\t--debug flag=HEX\tSet Debug flags, HEX is bitmask of: %s", TL_LOG_NEWLINE);
	printf("\t\t 0x0001\tGateway related logs%s", TL_LOG_NEWLINE);
	printf("\t\t 0x0002\tLamp related logs%s", TL_LOG_NEWLINE);	
	printf("\t\t 0x0004\tTimer related logs%s", TL_LOG_NEWLINE);	
	printf("\t\t 0x0010\tInit process%s", TL_LOG_NEWLINE);			
	printf("\t\t 0x0020\tPackets receive%s", TL_LOG_NEWLINE);	
	printf("\t\t 0x0040\tCfg Packets send%s", TL_LOG_NEWLINE);	
	printf("\t\t 0x0080\tCmd Packets send%s", TL_LOG_NEWLINE);	
	printf("\t\t 0x0100\tRequest Packets send%s", TL_LOG_NEWLINE);		
	printf("\t\t 0x0200\tDump Packets contents%s", TL_LOG_NEWLINE);	
	printf("\t\t 0x0800\tConfig files parse%s", TL_LOG_NEWLINE);	
	printf("\t\t 0x1000\tManagement requests%s", TL_LOG_NEWLINE);	
	printf("\t--debug dest=NAME\tSet Debug destination, where NAME can be:%s", TL_LOG_NEWLINE);
	printf("\t\tNULL\t\tDo not log%s", TL_LOG_NEWLINE);
	printf("\t\tConsole\t\tLog to console%s", TL_LOG_NEWLINE);
	printf("\t\tSyslog\t\tLog to Syslog%s", TL_LOG_NEWLINE);
}

void print_help(char *pPname)
{
	printf("Usage: %s [OPIONS]%s", pPname, TL_LOG_NEWLINE);
	printf("\t-h, --help\t\tHelp%s", TL_LOG_NEWLINE);
	printf("\t-v, --version\t\tVersion information%s", TL_LOG_NEWLINE);
	print_help_debug();
	fflush(stdout);
}

int get_argument_value(char *szString, char **pszValue)
{
	int i=0;
	*pszValue = NULL;
	while(szString[i] && szString[i] != '=')i++;
	if(szString[i])
	{
		szString[i] = 0;
		*pszValue = &(szString[i+1]);
	}
	return OK;
}

int get_client_fd()
{
	if(-1 != iClientFd)
		return iClientFd;
	iClientFd = tl_mgmt_connect();	 
	return iClientFd;
}

void set_debug(char *szArgument, char *szValue)
{
	uint8_t iCommand, *pData = NULL, iDest, iLen, iFail=0;
	uint32_t iFlags;
	
	if(0 == strncmp(szArgument, "flag", strlen("flag")))
	{
		iCommand = TL_API_SET_DEBUG_FLAG;
		iLen = 4;
		pData = (uint8_t*)&iFlags;
		iFlags = strtol(szValue, NULL, 16);
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
	}else if(0 == strncmp(szArgument, "get", strlen("get")))
	{
		iCommand = TL_API_SET_DEBUG_GET;
		iLen = 0;	
		return;
	}else
		iFail++;
	
	if(0 == iFail)
	{
		tl_mgmt_send(get_client_fd(), iCommand, iLen, pData);
		return;
	}
	
	printf("Uknown debug arguments %s,%s, Usage:", szArgument, szValue);
	print_help_debug();
	fflush(stdout);
}

int main(int argc, char **argv)
{
	int iOpt;
	char *szSubOpts;
	struct option 	stLongOptions[] =
					{
						{"help", 0, 0, 1},
						{"version", 0, 0, 2},
						{"debug", 1, 0, 3},
						{0, 0, 0, 0}
					};
	
	while ( (iOpt = getopt_long( argc, argv, "vhd", stLongOptions, NULL )) != -1 )
	{
		switch(iOpt)
		{
			case 3: /* debug_flag */
				if(OK == get_argument_value(optarg,&szSubOpts))
					set_debug(optarg,szSubOpts);
				break;
			case 2:
			case 'v':
				print_help(argv[0]);
				exit(1);				
			case 1:
			case 'h':	
			default: /* '?' */
				print_help(argv[0]);
				exit(1);
		}
	}

	return 0;
}

