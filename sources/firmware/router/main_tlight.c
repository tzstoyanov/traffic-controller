#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_serial.h"
#include "tl_program.h"
#include "tl_packets.h"
#include "tl_controlers.h"
#include "tl_config.h"
#include "tl_api.h"

static tl_global_t stGlobal;

void print_help(char *pPname)
{
	printf("Usage: %s [OPIONS]\n\r", pPname);
	printf("\t-h, --help\t\tHelp\n\r");
	printf("\t-v, --version\t\tVersion information\n\r");
	printf("\t-c, --config FILENAME\t\tMain configuration file\n\r");
}

int32_t tl_wd_thread(thread_t *pThread)
{
	int i;
	struct timeval stDelay;
	tl_global_t *pGlobal = (tl_global_t*)pThread->pArg;
	pGlobal->pWDThread = NULL;

	for(i=0; i<TL_MAX_CONTROLERS; i++)
	{
		if(NULL == pGlobal->arrDevices[i])
			break;
		if(TL_MODULE_GATE != pGlobal->arrDevices[i]->iType)
			continue;
		tl_ask_controler_state(i);
	}

	stDelay.tv_sec = pGlobal->iWdInterval / 1000;
	stDelay.tv_usec = 1000*(pGlobal->iWdInterval % 1000);
	pGlobal->pWDThread = thread_timer_add(&(pGlobal->stMaintThread), tl_wd_thread, pGlobal, &stDelay); 
	return 1;
}

tl_global_t *tl_get_global()
{
	return &stGlobal;
}

int main(int argc, char **argv)
{
	int iOpt;
	struct timeval stWDDelay;
	struct option 	stLongOptions[] =
					{
						{"help", 0, 0, 1},
						{"version", 0, 0, 2},
						{"config", 1, 0, 3},
						{0, 0, 0, 0}
					};

	bzero(&stGlobal, sizeof(tl_global_t));

	while ( (iOpt = getopt_long( argc, argv, "c:vh", stLongOptions, NULL )) != -1 )
	{
		switch(iOpt)
		{
			case 3:
			case 'c':
				if(OK != tl_parse_cfg_file(optarg, &stGlobal));
				{
					printf("\n\r Error parsing the configuration file %s\n\r", optarg);
					exit(1);
				}
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
	thread_main_init(&(stGlobal.stMaintThread));

	tl_check_controlers();

	stWDDelay.tv_sec = stGlobal.iWdInterval / 1000;
	stWDDelay.tv_usec = 1000*(stGlobal.iWdInterval % 1000);
	stGlobal.pWDThread = thread_timer_add(&(stGlobal.stMaintThread), tl_wd_thread, &stGlobal, &stWDDelay);

	thread_main_run(&(stGlobal.stMaintThread));

	return 0;
}

