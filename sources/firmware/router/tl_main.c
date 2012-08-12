#include "os_linux.h"
#include "linux_thread.h"
#include "linux_sem.h"
#include "tl_types.h"
#include "tl_packets_api.h"
#include "lib_tl_log.h"
#include "tl_api.h"
#include "tl_serial.h"
#include "tl_program.h"
#include "tl_controlers.h"
#include "lib_tl_config.h"
#include "lib_tl_packets.h"
#include "tl_main.h"

static tl_global_t stGlobal;

void print_help(char *pPname)
{
	printf("Usage: %s [OPIONS]%s", pPname, TL_LOG_NEWLINE);
	printf("\t-h, --help\t\tHelp%s", TL_LOG_NEWLINE);
	printf("\t-v, --version\t\tVersion information%s", TL_LOG_NEWLINE);
	printf("\t-c, --config FILENAME\tConfiguration file%s", TL_LOG_NEWLINE);
	printf("\t-d, --daemod\t\tRun in daemon mode%s", TL_LOG_NEWLINE);
	printf("\t--debug HEX\t\tDebug flags, HEX is bitmask of:%s", TL_LOG_NEWLINE);	
	printf("\t\t %08X\tGateway related logs%s", TL_DEBUG_GATE, TL_LOG_NEWLINE);
	printf("\t\t %08X\tLamp related logs%s", TL_DEBUG_LAMP, TL_LOG_NEWLINE);	
	printf("\t\t %08X\tTimer related logs%s", TL_DEBUG_TIMER, TL_LOG_NEWLINE);	
	printf("\t\t %08X\tInit process%s", TL_DEBUG_INIT, TL_LOG_NEWLINE);			
	printf("\t\t %08X\tPackets receive%s", TL_DEBUG_RX, TL_LOG_NEWLINE);	
	printf("\t\t %08X\tCfg Packets send%s", TL_DEBUG_TX_CFG, TL_LOG_NEWLINE);	
	printf("\t\t %08X\tCmd Packets send%s", TL_DEBUG_TX_CMD, TL_LOG_NEWLINE);	
	printf("\t\t %08X\tRequest Packets send%s", TL_DEBUG_TX_RQST, TL_LOG_NEWLINE);		
	printf("\t\t %08X\tDump Packets contents%s", TL_DEBUG_PKT_DUMP, TL_LOG_NEWLINE);	
	printf("\t\t %08X\tConfig files parse%s", TL_DEBUG_CONFIG, TL_LOG_NEWLINE);	
	printf("\t\t %08X\tManagement requests%s", TL_DEBUG_API, TL_LOG_NEWLINE);
	
	fflush(stdout);
}

void tl_check_system_status()
{
	uint32_t iCritical=0, iWarning = 0, i;
	uint32_t iOldCritical=0, iOldWarning = 0;
	tl_global_t *pGlobal = tl_get_global();
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();
	
	for(i=1; i<TL_MAX_CONTROLERS; i++)
	{
		if(NULL == pDbGlobal->arrDevices[i])
			break;
		if(TL_MODULE_MASTER == pDbGlobal->arrDevices[i]->iType)
			continue;	
		if(pDbGlobal->arrDevices[i]->iWdMiss >= pDbGlobal->iDeadCount)
		{	
			if(pDbGlobal->arrDevices[i]->iWdMiss == pDbGlobal->iDeadCount)
				tl_log_error("Status for %s not updated for long time", 
								pDbGlobal->arrDevices[i]->szName);
			if(	pDbGlobal->arrDevices[i]->iCritical)
				iCritical++;
			else
				iWarning++;	
		}
		else if( TL_MODULE_ERR_OK != pDbGlobal->arrDevices[i]->iStatus)
		{
			if(	pDbGlobal->arrDevices[i]->iCritical && 
				TL_MODULE_ERR_HW_WARN != pDbGlobal->arrDevices[i]->iStatus &&
				pDbGlobal->arrDevices[i]->iErrorCount >= pDbGlobal->iDeadCount)
				iCritical++;
			else
				iWarning++;
		}

	}
	iOldCritical = pGlobal->iCritError;
	iOldWarning = pGlobal->iWarning;
	pGlobal->iCritError = iCritical;
	pGlobal->iWarning = iWarning;	
	if(iOldCritical && 0 == iCritical)	
	{
		tl_log_info("Restore from critical error, reset current program");
		tl_program_reset(pGlobal->pCurrentProgram);	
	}else if(iCritical && 0 == iOldCritical)
	{
		tl_send_failsafe();
		tl_log_error("Critical error detected, enter in FailSafe mode");
		tl_program_pause(pGlobal->pCurrentProgram);
	}else if(iCritical > iOldCritical)
		tl_log_error("New critical error detected");
	if( iWarning > iOldWarning )	
		tl_log_error("New warning detected");
	else if(0 == iWarning && iOldWarning)
		tl_log_info("All warnings cleared");

}

tl_global_t *tl_get_global()
{
	return &stGlobal;
}

void tl_exit_callback(void *pCookie)
{
	tl_log_info("Exiting");
	exit(1);
}

int32_t tl_wd_thread(thread_t *pThread)
{
	int i;
	struct timeval stDelay;
	tl_global_config_t *pDbGlobal = (tl_global_config_t*)pThread->pArg;
	tl_global_t *pGlobal = tl_get_global();
	pGlobal->pWDThread = NULL;

	for(i=1; i<TL_MAX_CONTROLERS; i++)
	{
		if(NULL == pDbGlobal->arrDevices[i])
			break;
		if(TL_MODULE_MASTER == pDbGlobal->arrDevices[i]->iType)
			continue;		
		pDbGlobal->arrDevices[i]->iWdMiss++;
		if(TL_MODULE_GATE != pDbGlobal->arrDevices[i]->iType)
			continue;
		tl_ask_controler_state(i);
	}
	for(i=1; i<TL_MAX_CONTROLERS; i++)
	{
		if(NULL == pDbGlobal->arrDevices[i])
			break;
		if(pDbGlobal->arrDevices[i]->iSendConfig)
		{
			tl_send_controlers_config(i);
			pDbGlobal->arrDevices[i]->iSendConfig = 0;
		}
	}	
	stDelay.tv_sec = pDbGlobal->iWdInterval / 1000;
	stDelay.tv_usec = 1000*(pDbGlobal->iWdInterval % 1000);
	pGlobal->pWDThread = thread_timer_add(&(pGlobal->stMaintThread), tl_wd_thread, pDbGlobal, &stDelay); 
	return 1;
}

void tl_daemonzie(int iSem)
{
	pid_t pid, sid;

	pid = fork();
	if(pid < 0)
		exit(1);
	if(pid > 0)
	{
		sem_take(iSem);
		sem_delete(iSem);
		exit(0);
	}	
	sid = setsid();
	if(sid < 0)
		exit(1);
}

void tl_mask_signals()
{
	sigset_t sig_mask;
	
    sigfillset(&sig_mask);
    sigdelset(&sig_mask, SIGINT);
    sigdelset(&sig_mask, SIGHUP);
    sigdelset(&sig_mask, SIGTERM);
    sigdelset(&sig_mask, SIGPIPE);
    sigdelset(&sig_mask, SIGUSR1);
    sigdelset(&sig_mask, SIGUSR2);
    
    /*pthread_sigmask(SIG_SETMASK, &sig_mask, NULL);*/
}

int main(int argc, char **argv)
{
	int iOpt, iDaemon=0, iSem=-1, iCfg = 0;
	struct timeval stWDDelay;
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();
	struct option 	stLongOptions[] =
					{
						{"help", 0, 0, 1},
						{"version", 0, 0, 2},
						{"config", 1, 0, 3},
						{"debug", 1, 0, 4},
						{"daemon", 0, 0, 5},
						{0, 0, 0, 0}
					};

	bzero(&stGlobal, sizeof(tl_global_t));
	bzero(pDbGlobal, sizeof(tl_global_config_t));
	stGlobal.iApiFd = -1;
	pDbGlobal->stPort.iNetFd = -1;
	pDbGlobal->stPort.iAccFd = -1;

	tl_log_init(TL_LOG2CONSOLE, TL_LOG_PREFIX);
	
	while ( (iOpt = getopt_long( argc, argv, "c:vhd", stLongOptions, NULL )) != -1 )
	{
		switch(iOpt)
		{
			case 3:
			case 'c':
				if(OK != tl_parse_cfg_file(optarg, pDbGlobal, tl_NewControler))
				{
					tl_log_error("Error parsing the configuration file %s", optarg);
					exit(1);
				}
				iCfg = 1;
				break;
			case 4: /* debug */
				tl_debug_switch_set(strtol(optarg, NULL, 16));
				break;
			case 5:
			case 'd': /* daemon mode */
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
	if(iDaemon)
	{
		iSem = sem_create(SEM_EMPTY);
		if(iSem < 0)
			exit(1);
		tl_daemonzie(iSem);
	}
	thread_main_init(&(stGlobal.stMaintThread));
	if(OK != tl_com_port_open(&(pDbGlobal->stPort)))
	{
		if(iSem != -1)
			sem_give(iSem);
		exit(1);
	}
	if(pDbGlobal->stPort.iNetFd > 0)
		pDbGlobal->stPort.pReadThread = 
				thread_read_add(&(stGlobal.stMaintThread), tl_com_read, &(pDbGlobal->stPort), pDbGlobal->stPort.iNetFd);
	else
		pDbGlobal->stPort.pAcceptThread = 
				thread_read_add(&(stGlobal.stMaintThread), tl_com_accept, &(pDbGlobal->stPort), pDbGlobal->stPort.iAccFd);
						
	if(OK != tl_api_init())
	{
		if(iSem != -1)
			sem_give(iSem);
		exit(1);
	}
	tl_check_controlers();
	tl_send_controlers_config(TL_ADDRESS_BCAST);

	stWDDelay.tv_sec = pDbGlobal->iWdInterval / 1000;
	stWDDelay.tv_usec = 1000*(pDbGlobal->iWdInterval % 1000);
	stGlobal.pWDThread = thread_timer_add(&(stGlobal.stMaintThread), tl_wd_thread, pDbGlobal, &stWDDelay);

	if(iSem != -1)
		sem_give(iSem);

	if(TL_IS_DEBUG(INIT))
		tl_log_info("Init OK, run main loop");
	
	thread_main_run(&(stGlobal.stMaintThread));

	return 0;
}

