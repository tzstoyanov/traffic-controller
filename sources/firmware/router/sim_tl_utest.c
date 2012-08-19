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

typedef int (*sim_utest_hook_t)(tl_sim_global_t *pGlobal);
int sim_tl_utest_start();

int sim_utest_normal_init(tl_sim_global_t *pSim)
{
	int i;
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();		
	tl_sim_controler_t *pSimControler = NULL;

	for(i=1; i<TL_MAX_CONTROLERS; i++)
	{
		if(NULL == pDbGlobal->arrDevices[i])
			break;	
		pSimControler = (tl_sim_controler_t *)pDbGlobal->arrDevices[i]->pData;
		pSimControler->iSimStatus = -1;
		pSimControler->iSimSegments = -1;
	}
	
	return OK;
}

int sim_utest_normal_step(tl_sim_global_t *pSim)
{	
	int iRet;
	if(3 <=  pSim->iTestStepCount)
	{
		iRet = sim_tl_check_status(0, 0, 0, TL_PROGRAM_RUNNING);
		tl_log_info(" ... %s", OK==iRet?"PASSED":"FAILED");
		return OK;
	}
	return TL_CONTINUE;
}

int sim_utest_timerstate_init(tl_sim_global_t *pSim)
{
	int i;
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();		
	tl_sim_controler_t *pSimControler = NULL;
	
	tl_log_info("");
	for(i=1; i<TL_MAX_CONTROLERS; i++)
	{
		if(NULL == pDbGlobal->arrDevices[i])
			break;	
		if(TL_MODULE_TIMER != pDbGlobal->arrDevices[i]->iType)
			continue; 
		pSim->iDut = i;	
		pSimControler = (tl_sim_controler_t *)pDbGlobal->arrDevices[i]->pData;
		pSimControler->iSimStatus = TL_MODULE_STATE_INIT;	
		bzero(pSimControler->arrCfgReceived, sizeof(pSimControler->arrCfgReceived));
		break;
	}

	return OK;
}

int sim_utest_timerstate_step(tl_sim_global_t *pSim)
{
	int iRet;
	tl_sim_controler_t *pSimControler;
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();
	
	if(0 == (1+pSim->iTestStepCount)%3)
	{
		pSimControler = (tl_sim_controler_t *)pDbGlobal->arrDevices[pSim->iDut]->pData;
		
		tl_log_info_nonr("\t Status %s ... ",  tl_status2string(pSimControler->iSimStatus));
		
		if(TL_MODULE_STATE_OK == pSimControler->iSimStatus)
			iRet = sim_tl_check_status(0, 0, 0, TL_PROGRAM_RUNNING);		
		else
			iRet = sim_tl_check_status(1, 0, 1, TL_PROGRAM_RUNNING);	
		if(OK != iRet)
			tl_log_info("FAILED, unexpected system state");

		if(TL_MODULE_STATE_INIT == pSimControler->iSimStatus)
		{
			if(0 == pSimControler->arrCfgReceived[TL_CFGID_QINTERVAL-1])
			{
				iRet = ERROR;
				tl_log_info("FAILED, Config qInterval not received");
			}
			if(0 == pSimControler->arrCfgReceived[TL_CFGID_DEADCOUNT-1])
			{
				iRet = ERROR;
				tl_log_info("FAILED, Config DeadCount not received");
			}
			if(0 == pSimControler->arrCfgReceived[TL_CFGID_COMTIMEOUT-1])
			{
				iRet = ERROR;
				tl_log_info("FAILED, Config CommTimeout not received");			
			}
			if(0 != pSimControler->arrCfgReceived[TL_CFGID_SLAVES-1])
			{
				iRet = ERROR;
				tl_log_info("FAILED, Config Slaves not received");				
			}		
		}
		if(OK == iRet)
			tl_log_info("PASSED");
		pSimControler->iSimStatus++;
		if(TL_MODULE_STATE_MISMATCH == pSimControler->iSimStatus)
			pSimControler->iSimStatus = TL_MODULE_STATE_OK;	
		if(TL_MODULE_STATE_INIT == pSimControler->iSimStatus)
		{
			pSimControler->iSimStatus = -1;	
			return OK;
		}	
	}
	return TL_CONTINUE;
}

struct
{
	sim_utest_hook_t fHookInit;
	sim_utest_hook_t fHookStep;
	char *szTestName;
}arrSimUtest[]={
	{sim_utest_normal_init, sim_utest_normal_step, "Normal operation"},
	{sim_utest_timerstate_init, sim_utest_timerstate_step, "Timer Status Change"}
};

int32_t sim_tl_utest_step(thread_t *pThread)
{
	int iRet;
	tl_sim_global_t *pSim = tl_get_global_sim();
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();		
	int iTestsCount = (sizeof(arrSimUtest)/sizeof(arrSimUtest[0]));
	
	iRet = arrSimUtest[pSim->iTestInProgress].fHookStep(pSim);
	if(TL_CONTINUE != iRet)
	{
		pSim->iTestInProgress++;
		if(pSim->iTestInProgress >= iTestsCount)	
		{
			tl_log_info("All tests executed, exiting");
			exit(1);
		}
		pSim->iTestStepDelay = pDbGlobal->iWdInterval;
		pSim->iTestStepCount = 0;
		sim_tl_utest_start();
	}
	else
	{
		pSim->iTestStepCount++;
		pSim->pUTThread = thread_timer_add(&(pSim->stMaintThread), 
									sim_tl_utest_step, NULL, pSim->iTestStepDelay);
	}	
	return 1;
}

int sim_tl_utest_start()
{
	tl_sim_global_t *pSim = tl_get_global_sim();
	
	tl_log_info_nonr("Starting test [%s]", arrSimUtest[pSim->iTestInProgress].szTestName);
	arrSimUtest[pSim->iTestInProgress].fHookInit(pSim);

	pSim->pUTThread = thread_timer_add(&(pSim->stMaintThread), 
	                        sim_tl_utest_step, NULL, pSim->iTestStepDelay); 
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

