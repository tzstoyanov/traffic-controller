#include "os_linux.h"
#include "linux_thread.h"
#include "linux_sem.h"
#include "tl_types.h"
#include "tl_packets_api.h"
#include "tl_serial_api.h"
#include "tl_program_api.h"
#include "tl_controlers_api.h"
#include "tl_api.h"
#include "lib_tl_log.h"
#include "lib_tl_config.h"
#include "lib_tl_com.h"
#include "lib_tl_packets.h"
#include "lib_tl_client.h"
#include "sim_tl_main.h"

tl_sim_global_t stSimGlobal;

void print_help(char *pPname)
{
	tl_log_info("Usage: %s [OPIONS]", pPname);
	tl_log_info("\t-h, --help\t\tHelp");
	tl_log_info("\t-v, --version\t\tVersion information");
	tl_log_info("\t-c, --config FILENAME\tConfiguration file");
	tl_log_info("\t-u, --unit\t\tRun unit test");
	tl_log_info("\t-d, --daemon\t\tRun in daemon mode");	
	tl_log_info("\t-l, --log DEST\t\tLog destination, DEST is:");
	tl_print_help_debug_dest();
	tl_log_info("\t--debug HEX\t\tDebug flags, HEX is bitmask of:");
	tl_print_help_debug_flag();
}

int32_t sim_tl_wd_thread(thread_t *pThread)
{
	int i;
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
		if(pDbGlobal->arrDevices[i]->iWdMiss == pDbGlobal->iDeadCount)
			tl_log_info("%s WatchDog %d, eneter in FailSafe mode", 
				pDbGlobal->arrDevices[i]->szName, pDbGlobal->arrDevices[i]->iWdMiss);
	}

	pGlobal->pWDThread = thread_timer_add(&(pGlobal->stMaintThread), 
								sim_tl_wd_thread, pGlobal, pDbGlobal->iWdInterval); 
	return 1;
}

tl_sim_global_t *tl_get_global_sim()
{
	return &stSimGlobal;
}

int sim_get_client_fd()
{
	tl_sim_global_t *pGlobal = tl_get_global_sim();
	if(-1 != pGlobal->iApiFd)
		return pGlobal->iApiFd;
	pGlobal->iApiFd = tl_mgmt_connect();	 
	return pGlobal->iApiFd;
}

int main(int argc, char **argv)
{
	int iOpt, iCfg = 0, iDaemon=0, iSem=-1;
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();	
	struct option 	stLongOptions[] =
					{
						{"help", 0, 0, 1},
						{"version", 0, 0, 2},
						{"config", 1, 0, 3},
						{"utest", 0, 0, 4},
						{"log", 1, 0, 5},
						{"debug", 1, 0, 6},
						{"daemon", 0, 0, 7},
						{0, 0, 0, 0}
					};

	bzero(&stSimGlobal, sizeof(tl_sim_global_t));
	bzero(pDbGlobal, sizeof(tl_global_config_t));
	stSimGlobal.iApiFd = -1;
	
	tl_log_init(TL_LOG2CONSOLE, TL_SIM_LOG_PREFIX);
	tl_debug_switch_set(TL_DEBUG_LAMP|TL_DEBUG_TIMER);
	
	while ( (iOpt = getopt_long( argc, argv, "c:vhuld", stLongOptions, NULL )) != -1 )
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
				stSimGlobal.iUtest = 1;
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
			case 'd':
			case 7: /* daemon */
				iDaemon = 1;
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
	if(iDaemon)
	{
		iSem = sem_create(SEM_EMPTY);
		if(iSem < 0)
			exit(1);
		tl_daemonzie(iSem);
	}

	thread_main_init(&(stSimGlobal.stMaintThread));
	if(OK != tl_com_sim_connect(&(pDbGlobal->stPort)))
	{
		tl_log_error("Failed to open socket to main program");
		if(iSem != -1)
			sem_give(iSem);		
		exit(1);
	}
	
	pDbGlobal->stPort.pReadThread  = 
		thread_read_add(&(stSimGlobal.stMaintThread), sim_tl_com_read, &(pDbGlobal->stPort), pDbGlobal->stPort.iNetFd);

	stSimGlobal.pWDThread = thread_timer_add(&(stSimGlobal.stMaintThread), 
								sim_tl_wd_thread, &stSimGlobal, pDbGlobal->iWdInterval);

	stSimGlobal.iApiFd = tl_mgmt_connect();
	sim_tl_utest_init();
	
	tl_clock_gettime(&(pDbGlobal->stStartTime));

	if(iSem != -1)
		sem_give(iSem);

	if(TL_IS_DEBUG(INIT))
		tl_log_info("Init OK, run main loop");
	
	thread_main_run(&(stSimGlobal.stMaintThread));

	return 0;
}

