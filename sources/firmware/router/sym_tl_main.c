#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_serial_api.h"
#include "tl_packets_api.h"
#include "tl_program_api.h"
#include "tl_controlers_api.h"
#include "lib_tl_log.h"
#include "lib_tl_config.h"
#include "lib_tl_com.h"
#include "lib_tl_packets.h"
#include "sym_tl_main.h"

tl_sym_global_t stSymGlobal;
uint32_t iTlightDebugSwitch = 0;

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

int tl_controler_matster(uint8_t iAddress, t_can_packet_t *pPacket)
{
	
	switch(pPacket->iCommand)
	{
		case TL_CMD_CONFIG:
			break;
		case TL_CMD_STATE:
			break;
		case TL_CMD_WD_REQUEST:
			break;
		case TL_CMD_WD_RESPONCE:
		case TL_CMD_WD_RESPONCE_F:
		default:
			tl_log_error("Received unexpected packet from %d: %d", iAddress, pPacket->iCommand);
			break;
	}
	return OK;
}

tl_controler_t *sym_tl_NewControler(uint8_t iType)
{
	tl_controler_t *pControler = NULL;
	switch(iType)
	{
		case TL_MODULE_MASTER:
			pControler = calloc(1, sizeof(tl_controler_t));
			pControler->iStatus = TL_MODULE_ERR_INIT;
			pControler->fPktProcess = tl_controler_matster;
			pControler->iCritical = 1;
			break;
		case TL_MODULE_GATE:
			pControler = calloc(1, sizeof(tl_controler_t));
			pControler->iStatus = TL_MODULE_ERR_INIT;
			pControler->iCritical = 0;
			break;
		case TL_MODULE_LAMP:
			pControler = calloc(1, sizeof(tl_controler_lamp_t));				
			pControler->iStatus = TL_MODULE_ERR_INIT;
			pControler->iCritical = 1;
			break;
		case TL_MODULE_TIMER:
			pControler = calloc(1, sizeof(tl_controler_timer_t));		
			pControler->iStatus = TL_MODULE_ERR_INIT;
			pControler->iCritical = 0;
			break;
		default:
			tl_log_error("Uknown controler type %d", iType);
			return NULL;
	}
	if(NULL != pControler)
		pControler->iType = iType;
	return pControler;	
}

int32_t sym_tl_wd_thread(thread_t *pThread)
{
	int i;
	struct timeval stDelay;
	tl_sym_global_t *pGlobal = (tl_sym_global_t*)pThread->pArg;
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();		
	pGlobal->pWDThread = NULL;

	for(i=1; i<TL_MAX_CONTROLERS; i++)
	{
		if(NULL == pDbGlobal->arrDevices[i])
			break;
		if(TL_MODULE_GATE != pDbGlobal->arrDevices[i]->iType)
			continue;

			

	}

	stDelay.tv_sec = pDbGlobal->iWdInterval / 1000;
	stDelay.tv_usec = 1000*(pDbGlobal->iWdInterval % 1000);
	pGlobal->pWDThread = thread_timer_add(&(pGlobal->stMaintThread), sym_tl_wd_thread, pGlobal, &stDelay); 
	return 1;
}

int32_t sym_tl_com_read(thread_t *pThread)
{
	tl_com_t *pCom = (tl_com_t*) pThread->pArg;
	
	pCom->pReadThread  = NULL;

	tl_com_pkt_process(pCom);

	pCom->pReadThread  = thread_read_add(pThread->pMain, sym_tl_com_read, pCom, pCom->iNetFd);
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
						{0, 0, 0, 0}
					};

	bzero(&stSymGlobal, sizeof(tl_sym_global_t));
	bzero(pDbGlobal, sizeof(tl_global_config_t));
	
	tl_log_init(TL_LOG2CONSOLE, TL_SYM_LOG_PREFIX);
	
	while ( (iOpt = getopt_long( argc, argv, "c:vhul", stLongOptions, NULL )) != -1 )
	{
		switch(iOpt)
		{
			case 3:
			case 'c':
				if(OK != tl_parse_cfg_file(optarg, pDbGlobal, sym_tl_NewControler))
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
	thread_main_init(&(stSymGlobal.stMaintThread));
	if(OK != tl_com_sym_open(&(pDbGlobal->stPort)))
	{
		tl_log_error("Failed to open socket to main program");
		exit(1);
	}
	pDbGlobal->stPort.pReadThread  = 
		thread_read_add(&(stSymGlobal.stMaintThread), sym_tl_com_read, &(pDbGlobal->stPort), pDbGlobal->stPort.iNetFd);

	stWDDelay.tv_sec = pDbGlobal->iWdInterval / 1000;
	stWDDelay.tv_usec = 1000*(pDbGlobal->iWdInterval % 1000);
	stSymGlobal.pWDThread = thread_timer_add(&(stSymGlobal.stMaintThread), sym_tl_wd_thread, &stSymGlobal, &stWDDelay);

	if(TL_IS_DEBUG(INIT))
		tl_log_info("Init OK, run main loop");
	
	thread_main_run(&(stSymGlobal.stMaintThread));

	return 0;
}

