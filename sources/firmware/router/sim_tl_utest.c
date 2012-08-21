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

int sim_utest_state_reset(tl_sim_global_t *pSim)
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
		pSimControler->iAdaptiveStateInit = 0;
		pSimControler->iSimStatusBlock = 0;
	}
	
	return OK;
}


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
	if(4 <=  pSim->iTestStepCount)
	{
		iRet = sim_tl_check_status(0, 0, 0, TL_PROGRAM_RUNNING);
		tl_log_info(" ... %s", OK==iRet?"PASSED":"FAILED");
		return OK;
	}
	return TL_CONTINUE;
}

void sim_utest_set_init_state(tl_sim_controler_t *pSim)
{
	pSim->iWdInterval=0;
	pSim->iDeadCount = 0;
	pSim->iComTimeout = 0;
	pSim->iSlavesAttached = 0;
	bzero(pSim->arrSlaves, sizeof(pSim->arrSlaves));
	pSim->iAdaptiveStateInit = 1;
	bzero(pSim->arrCfgReceived, sizeof(pSim->arrCfgReceived));	
	pSim->iSimStatus = TL_MODULE_STATE_INIT;	
}

int sim_utest_modulestate_init(tl_sim_global_t *pSim, uint8_t iType, uint8_t iDutId, uint8_t iState)
{
	int i;
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();		
	tl_sim_controler_t *pSimControler = NULL;
	
	for(i=1; i<TL_MAX_CONTROLERS; i++)
	{
		if(NULL == pDbGlobal->arrDevices[i])
			break;	
		if(iType != pDbGlobal->arrDevices[i]->iType)
			continue; 
		pSim->arrDut[iDutId] = i;	
		pSimControler = (tl_sim_controler_t *)pDbGlobal->arrDevices[i]->pData;
		pSimControler->iSimStatusBlock = 0;
		if(TL_MODULE_STATE_INIT == iState)
			sim_utest_set_init_state(pSimControler);
		else
			pSimControler->iSimStatus = iState;				
		break;
	}

	return OK;
}

void tl_sim_calc_dut_status(uint8_t iDut, uint8_t *pCrit, uint8_t *pWar)
{
	tl_sim_controler_t *pSimControler;
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();

	pSimControler = (tl_sim_controler_t *)pDbGlobal->arrDevices[iDut]->pData;
	if(	TL_MODULE_STATE_OK == pSimControler->iSimStatus || 
		TL_MODULE_STATE_INIT == pSimControler->iSimStatus)
		return;
	if(!pDbGlobal->arrDevices[iDut]->iCritical || 
		TL_MODULE_STATE_HW_WARN == pSimControler->iSimStatus ||
		TL_MODULE_STATE_FAILSAFE == pSimControler->iSimStatus)	
		(*pWar)++;
	else
		(*pCrit)++;	
			
}

int sim_utest_modulestate_step(tl_sim_global_t *pSim)
{
	int iRet, iDut;
	uint8_t iDevError, iCritical, iWarning, iCrit, iWarn, iGateErr, iModOk; 
	tl_program_state_t iProgState;
	tl_sim_controler_t *pSimControler;
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();
	
	if(0 == (1+pSim->iTestStepCount)%4)
	{		
		pSimControler = (tl_sim_controler_t *)pDbGlobal->arrDevices[pSim->arrDut[0]]->pData;
		tl_log_info_nonr("\t Status %s ... ",  tl_status2string(pSimControler->iSimStatus));

		iDevError = 0;
		iCritical = 0;
		iWarning = 0;
		iGateErr = 0;
		iModOk = 0;		
		iProgState = TL_PROGRAM_RUNNING;
		for(iDut = 0; iDut <TL_SIM_MAX_DUT; iDut++)
		{
			if(! pSim->arrDut[iDut] )
				break;
			iCrit = 0;
			iWarn = 0;	
			pSimControler = (tl_sim_controler_t *)pDbGlobal->arrDevices[pSim->arrDut[iDut]]->pData;
			tl_sim_calc_dut_status(pSim->arrDut[iDut], &iCrit, &iWarn);
			iCritical += iCrit;
			iWarning += iWarn;
			if(TL_MODULE_GATE == pDbGlobal->arrDevices[pSim->arrDut[iDut]]->iType && 
				(iCrit || iWarn))
				iGateErr++;
			if(TL_MODULE_GATE != pDbGlobal->arrDevices[pSim->arrDut[iDut]]->iType && 
				!iCrit && !iWarn)
				iModOk++;	
		}
		if(iCritical)
		{
			iDevError = pSim->iLampCount + pSim->iTimerCount + iGateErr - iModOk;
			iWarning = iDevError - iCritical;
			iProgState = TL_PROGRAM_PAUSE;
		}else
			iDevError = iWarning;

		for(iDut = 0; iDut <TL_SIM_MAX_DUT; iDut++)
		{
			if(! pSim->arrDut[iDut] )
				break;
			pSimControler = (tl_sim_controler_t *)pDbGlobal->arrDevices[pSim->arrDut[iDut]]->pData;

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
				if(TL_MODULE_GATE != pDbGlobal->arrDevices[pSim->arrDut[iDut]]->iType)
				{
					if(0 != pSimControler->arrCfgReceived[TL_CFGID_SLAVES-1])
					{
						iRet = ERROR;
						tl_log_info("FAILED, Config Slaves received");				
					}
				}
				else
				{
					if(0 == pSimControler->arrCfgReceived[TL_CFGID_SLAVES-1])
					{
						iRet = ERROR;
						tl_log_info("FAILED, Config Slaves not received");			
					}				
				}				
			}
			
			pSimControler->iSimStatus++;
			if(TL_MODULE_STATE_MISMATCH == pSimControler->iSimStatus)
				pSimControler->iSimStatus = TL_MODULE_STATE_OK;	
			if(TL_MODULE_STATE_INIT == pSimControler->iSimStatus)
				sim_utest_set_init_state(pSimControler);
		}
		iRet = sim_tl_check_status(iDevError, iCritical, iWarning, iProgState);			
		if(OK != iRet)
			tl_log_info("FAILED, unexpected system state");

		if(OK == iRet)
			tl_log_info("PASSED");

		pSimControler = (tl_sim_controler_t *)pDbGlobal->arrDevices[pSim->arrDut[0]]->pData;
		if(TL_MODULE_STATE_INIT == pSimControler->iSimStatus)
		{
			pSimControler->iSimStatus = -1;	
			return OK;
		}	
	}
	return TL_CONTINUE;
}

int sim_utest_timerstate_init(tl_sim_global_t *pSim)
{
	bzero( pSim->arrDut, sizeof(pSim->arrDut));
	tl_log_info("");
	return sim_utest_modulestate_init(pSim, TL_MODULE_TIMER, 0, TL_MODULE_STATE_INIT);
}

int sim_utest_lampstate_init(tl_sim_global_t *pSim)
{
	bzero( pSim->arrDut, sizeof(pSim->arrDut));
	tl_log_info("");
	return sim_utest_modulestate_init(pSim, TL_MODULE_LAMP, 0, TL_MODULE_STATE_INIT);
}

int sim_utest_gatestate_init(tl_sim_global_t *pSim)
{
	bzero( pSim->arrDut, sizeof(pSim->arrDut));
	tl_log_info("");	
	return sim_utest_modulestate_init(pSim, TL_MODULE_GATE, 0, TL_MODULE_STATE_INIT);
}

int sim_utest_mixstate_init(tl_sim_global_t *pSim)
{
	bzero( pSim->arrDut, sizeof(pSim->arrDut));
	tl_log_info("");
	sim_utest_modulestate_init(pSim, TL_MODULE_GATE, 0, TL_MODULE_STATE_INIT);
	sim_utest_modulestate_init(pSim, TL_MODULE_TIMER, 1, TL_MODULE_STATE_HW_ERR);	
	sim_utest_modulestate_init(pSim, TL_MODULE_LAMP, 2, TL_MODULE_STATE_HW_WARN);	
	return OK;
}

int sim_utest_gatenoreply_init(tl_sim_global_t *pSim, uint8_t iCritical)
{
	int i;
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();
	tl_sim_controler_t *pSimControler = NULL;

	pSim->arrDut[0] = 0;
	for(i=1; i<TL_MAX_CONTROLERS; i++)
	{
		if(NULL == pDbGlobal->arrDevices[i])
			break;	
		if(TL_MODULE_GATE != pDbGlobal->arrDevices[i]->iType)
			continue; 
		if((iCritical && 0 == pDbGlobal->arrDevices[i]->iCritical) ||
			(0 == iCritical && pDbGlobal->arrDevices[i]->iCritical))
			continue;	
			
		pSim->arrDut[0] = i;
		pSimControler = (tl_sim_controler_t *)pDbGlobal->arrDevices[i]->pData;
		pSimControler->iSimStatusBlock = 0;
		sim_utest_set_init_state(pSimControler);
	}
	return OK;
}

int sim_utest_gatenoreply_critical_init(tl_sim_global_t *pSim)
{
	return sim_utest_gatenoreply_init(pSim, 1);
}

int sim_utest_gatenoreply_noncritical_init(tl_sim_global_t *pSim)
{
	return sim_utest_gatenoreply_init(pSim, 0);
}

int sim_utest_gatenoreply_step(tl_sim_global_t *pSim)
{
	uint8_t iDevError, iCritical, iWarning; 
	tl_program_state_t iProgState;
	tl_sim_controler_t *pSimControler;
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();
	
	if(0 == pSim->arrDut[0])
	{
		tl_log_info(" ... FAILED, No suitable configuration in system config file");
		return OK;
	}
	
	if(4 == pSim->iTestStepCount)
	{
		if(OK != sim_tl_check_status(0, 0, 0, TL_PROGRAM_RUNNING))
		{
			tl_log_info(" ... FAILED, unexpected system state");
			return OK;
		}
		pSimControler = (tl_sim_controler_t *)pDbGlobal->arrDevices[pSim->arrDut[0]]->pData;
		pSimControler->iSimStatusBlock = 1;
	}
	else if(8 == pSim->iTestStepCount)
	{
		pSimControler = (tl_sim_controler_t *)pDbGlobal->arrDevices[pSim->arrDut[0]]->pData;
		iCritical = 0;
		iWarning = 0;
		iProgState = TL_PROGRAM_RUNNING;
		if(pDbGlobal->arrDevices[pSim->arrDut[0]]->iCritical)
		{
			iDevError = pSim->iLampCount + pSim->iTimerCount + 1;
			iCritical = pSim->iLampCount + 1;
			iWarning = iDevError - iCritical;
			iProgState = TL_PROGRAM_PAUSE;
		}
		else
		{
			iDevError = pSimControler->iSlavesAttached + 1;
			iWarning = iDevError;
		}
		if(OK != sim_tl_check_status(iDevError, iCritical, iWarning, iProgState))
			tl_log_info(" ... FAILED, unexpected system state");
		else	
			tl_log_info(" ... PASSED");
			
		return OK;	
	}else if( 8 < pSim->iTestStepCount)
	{
		return OK;
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
	{sim_utest_timerstate_init, sim_utest_modulestate_step, "Timer Status Change"},
	{sim_utest_lampstate_init, sim_utest_modulestate_step, "Lamp Status Change"},
	{sim_utest_gatestate_init, sim_utest_modulestate_step, "Gateway Status Change"},
	{sim_utest_mixstate_init, sim_utest_modulestate_step, "Mixed Modules Status Change"},
	{sim_utest_gatenoreply_critical_init, sim_utest_gatenoreply_step, "Critical gate no reply"},
	{sim_utest_gatenoreply_noncritical_init, sim_utest_gatenoreply_step, "Non Critical gate no reply"}
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
	sim_utest_state_reset(pSim);
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
		
	tl_log_info("%d unit tests loaded", iTestsCount);
	tl_debug_switch_set(0);
	pGlobal->iTestStepDelay = pDbGlobal->iWdInterval;
	pGlobal->iTestInProgress=0;
	pGlobal->iTestStepCount = 0;
	sim_tl_utest_start();
	return OK;
}

