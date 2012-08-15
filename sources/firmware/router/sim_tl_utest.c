#include "os_linux.h"
#include "linux_thread.h"
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

typedef int (*sim_utest_hook_t)();
int sim_tl_utest_start();

int sim_utest_normal_init()
{
	
	return OK;
}

int sim_utest_normal_step()
{

	return OK;
}

struct
{
	sim_utest_hook_t fHookInit;
	sim_utest_hook_t fHookStep;
	char *szTestName;
}arrSimUtest[]={
	{sim_utest_normal_init, sim_utest_normal_step, "Normal operation"}
};

int32_t sim_tl_utest_step(thread_t *pThread)
{
	tl_sim_global_t *pGlobal = tl_get_global_sim();
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();		
	int iTestsCount = (sizeof(arrSimUtest)/sizeof(arrSimUtest[0]));
	
	if(OK != arrSimUtest[pGlobal->iTestInProgress].fHookStep())
	{
		pGlobal->iTestInProgress++;
		if(pGlobal->iTestInProgress >= iTestsCount)	
		{
			tl_log_info("All tests executed, exiting");
			exit(1);
		}
		pGlobal->iTestStepDelay = pDbGlobal->iWdInterval;
		pGlobal->iTestStepCount = 0;
		sim_tl_utest_start();
	}
	else
	{
		pGlobal->iTestStepCount++;
		pGlobal->pUTThread = thread_timer_add(&(pGlobal->stMaintThread), 
									sim_tl_utest_step, NULL, pGlobal->iTestStepDelay);
	}	
	return 1;
}

int sim_tl_utest_start()
{
	tl_sim_global_t *pGlobal = tl_get_global_sim();
	
	tl_log_info("");
	tl_log_info("Starting test [%s]", arrSimUtest[pGlobal->iTestInProgress].szTestName);
	arrSimUtest[pGlobal->iTestInProgress].fHookInit();

	pGlobal->pUTThread = thread_timer_add(&(pGlobal->stMaintThread), 
	                        sim_tl_utest_step, NULL, pGlobal->iTestStepDelay); 
	return OK;
}

int sim_tl_utest_init()
{	
	tl_sim_global_t *pGlobal = tl_get_global_sim();
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();	
	int iTestsCount = (sizeof(arrSimUtest)/sizeof(arrSimUtest[0]));
		
	if(0 == pGlobal->iUtest || 0 == iTestsCount)
		return OK;
		
	tl_log_info("Start %d unit tests", iTestsCount);
	tl_debug_switch_set(0);
	pGlobal->iTestStepDelay = pDbGlobal->iWdInterval;
	pGlobal->iTestInProgress=0;
	pGlobal->iTestStepCount = 0;
	sim_tl_utest_start();
	return OK;
}

