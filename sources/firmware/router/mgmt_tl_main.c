#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_packets_api.h"
#include "tl_api.h"
#include "lib_tl_log.h"
#include "lib_tl_client.h"
#include "mgmt_tl_main.h"

int iClientFd=-1;

void print_help(char *pPname)
{
	tl_log_info("%s %s; %s %s", pPname, MODULE_VERSION, __DATE__, __TIME__);
	tl_log_info("Usage: %s [OPIONS]", pPname);
	tl_log_info("\t-h, --help\tHelp");
	tl_log_info("\t-v, --version\tVersion information");
	tl_log_info("\t--syslog\tPrint output in syslog");
	tl_log_info("\t--stop\t\tStop main application");
	print_help_program();
	print_help_status();
	print_help_debug();
}

int get_argument_value(char *szString, char **pszValue)
{
	int i=0;
	*pszValue = NULL;
	if(NULL == szString)
		return OK;
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

int main(int argc, char **argv)
{
	int iOpt;
	char *szSubOpts;
	struct option 	stLongOptions[] =
					{
						{"help", 0, 0, 1},
						{"version", 0, 0, 2},
						{"debug", 1, 0, 3},
						{"status", 1, 0, 4},
						{"syslog", 0, 0, 5},
						{"program", 1, 0, 6},
						{"stop", 0, 0, 7},
						{0, 0, 0, 0}
					};
	
	tl_log_init(TL_LOG2CONSOLE, TL_MGMT_LOG_PREFIX);
	
	while ( (iOpt = getopt_long( argc, argv, "vhd", stLongOptions, NULL )) != -1 )
	{
		switch(iOpt)
		{
			case 3: /* debug */
				if(OK == get_argument_value(optarg,&szSubOpts))
					process_debug(optarg,szSubOpts);
				break;
			case 4: /* status */
				if(OK == get_argument_value(optarg,&szSubOpts))
					process_status(optarg,szSubOpts);
				break;
			case 5: /* syslog */
				tl_log_dest_set(TL_LOG2SYSLOG);
				break;	
			case 6: /* program */	
				if(OK == get_argument_value(optarg,&szSubOpts))
					process_program(optarg,szSubOpts);			
				break;
			case 7: /* stop */
				tl_mgmt_send(get_client_fd(), TL_API_STOP, NULL, 0);
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

