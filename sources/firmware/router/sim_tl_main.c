#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_packets_api.h"
#include "tl_serial_api.h"
#include "tl_program_api.h"
#include "tl_controlers_api.h"
#include "lib_tl_log.h"
#include "lib_tl_config.h"
#include "lib_tl_com.h"
#include "lib_tl_packets.h"
#include "sim_tl_main.h"

tl_sim_global_t stSimGlobal;

void print_help(char *pPname)
{
	printf("Usage: %s [OPIONS]%s", pPname, TL_LOG_NEWLINE);
	printf("\t-h, --help\t\tHelp%s", TL_LOG_NEWLINE);
	printf("\t-v, --version\t\tVersion information%s", TL_LOG_NEWLINE);
	printf("\t-c, --config FILENAME\tConfiguration file%s", TL_LOG_NEWLINE);
	printf("\t-u, --unit\t\tRun unit test%s", TL_LOG_NEWLINE);
	printf("\t-l, --log=DEST\t\tLog destination, DEST is:%s", TL_LOG_NEWLINE);
	printf("\t\t Null\tDo not log%s", TL_LOG_NEWLINE);
	printf("\t\t Console\tLog to console, default%s", TL_LOG_NEWLINE);
	printf("\t\t Syslog\tLog to syslog%s", TL_LOG_NEWLINE);
	
	fflush(stdout);
}

int32_t sim_tl_wd_thread(thread_t *pThread)
{
	int i;
	struct timeval stDelay;
	tl_sim_global_t *pGlobal = (tl_sim_global_t*)pThread->pArg;
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();	
		
	pGlobal->pWDThread = NULL;

	for(i=1; i<TL_MAX_CONTROLERS; i++)
	{
		if(NULL == pDbGlobal->arrDevices[i])
			break;
		if(TL_MODULE_GATE != pDbGlobal->arrDevices[i]->iType)
			continue;
		pDbGlobal->arrDevices[i]->iWdMiss++;
		if(pDbGlobal->arrDevices[i]->iWdMiss >= pDbGlobal->iDeadCount)
			tl_log_info("%s WatchDog %d, eneter in FailSafe mode", 
				pDbGlobal->arrDevices[i]->szName, pDbGlobal->arrDevices[i]->iWdMiss);
	}

	stDelay.tv_sec = pDbGlobal->iWdInterval / 1000;
	stDelay.tv_usec = 1000*(pDbGlobal->iWdInterval % 1000);
	pGlobal->pWDThread = thread_timer_add(&(pGlobal->stMaintThread), sim_tl_wd_thread, pGlobal, &stDelay); 
	return 1;
}

int main(int argc, char **argv)
{
	int iOpt, iCfg = 0;
	struct timeval stWDDelay;
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();	
	struct option 	stLongOptions[] =
					{
						{"help", 0, 0, 1},
						{"version", 0, 0, 2},
						{"config", 1, 0, 3},
						{"utest", 0, 0, 4},
						{"log", 1, 0, 5},
						{"debug", 1, 0, 6},
						{0, 0, 0, 0}
					};

	bzero(&stSimGlobal, sizeof(tl_sim_global_t));
	bzero(pDbGlobal, sizeof(tl_global_config_t));
	
	tl_log_init(TL_LOG2CONSOLE, TL_SIM_LOG_PREFIX);
	
	while ( (iOpt = getopt_long( argc, argv, "c:vhul", stLongOptions, NULL )) != -1 )
	{
		switch(iOpt)
		{
			case 3:
			case 'c':
				if(OK != tl_parse_cfg_file(optarg, pDbGlobal, sim_tl_NewControler))
				{
					tl_log_error("Error parsing the configuration file %s", optarg);
					exit(1);
				}
				iCfg = 1;
				break;
			case 4: /* unit */
			case 'u':
				break;
			case 5:
			case 'l':
				if(0 == strncmp(optarg, "NULL", strlen("NULL")))
					tl_log_dest_set(TL_LOG2NULL);
				else if(0 == strncmp(optarg, "Syslog", strlen("Syslog")))	
					tl_log_dest_set(TL_LOG2SYSLOG);
				else
					tl_log_dest_set(TL_LOG2CONSOLE);	
				break;	
			case 6: /* debug */
				tl_debug_switch_set(strtol(optarg, NULL, 16));
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
	if(0 == iCfg)
	{
		tl_log_error("No configuration file is specified");
		print_help(argv[0]);
		exit(1);
	}
	if(TL_COM_LOCALSOCK != pDbGlobal->stPort.iType)
	{
		tl_log_error("No local socket specified in config file");
		exit(1);
	}
	thread_main_init(&(stSimGlobal.stMaintThread));
	if(OK != tl_com_sim_connect(&(pDbGlobal->stPort)))
	{
		tl_log_error("Failed to open socket to main program");
		exit(1);
	}
	pDbGlobal->stPort.pReadThread  = 
		thread_read_add(&(stSimGlobal.stMaintThread), sim_tl_com_read, &(pDbGlobal->stPort), pDbGlobal->stPort.iNetFd);

	stWDDelay.tv_sec = pDbGlobal->iWdInterval / 1000;
	stWDDelay.tv_usec = 1000*(pDbGlobal->iWdInterval % 1000);
	stSimGlobal.pWDThread = thread_timer_add(&(stSimGlobal.stMaintThread), sim_tl_wd_thread, &stSimGlobal, &stWDDelay);

	if(TL_IS_DEBUG(INIT))
		tl_log_info("Init OK, run main loop");
	
	thread_main_run(&(stSimGlobal.stMaintThread));

	return 0;
}

