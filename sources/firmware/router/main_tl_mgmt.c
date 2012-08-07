#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_api.h"
#include "tl_log.h"
#include "tl_mgmt_client.h"

int iClientFd=-1;

void print_help_debug()
{
	printf("\t--debug flag=HEX\tSet Debug flags\n");
	printf("\t--debug dest=NAME\tSet Debug destination, where NAME can be:\n");
	printf("\t\tNULL\t\tDo not log\n");
	printf("\t\tConsole\t\tLog to console\n");
	printf("\t\tSyslog\t\tLog to Syslog\n");
}

void print_help(char *pPname)
{
	printf("Usage: %s [OPIONS]\n", pPname);
	printf("\t-h, --help\t\tHelp\n");
	printf("\t-v, --version\t\tVersion information\n");
	print_help_debug();
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
		iFlags = strtol(szArgument, NULL, 16);
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
	
	printf("\n\rUknown debug arguments %s,%s, Usage:", szArgument, szValue);
	print_help_debug();
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

