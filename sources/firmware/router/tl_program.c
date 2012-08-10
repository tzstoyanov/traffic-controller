#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_api.h"
#include "tl_serial.h"
#include "tl_program.h"
#include "tl_packets_api.h"
#include "tl_controlers.h"
#include "lib_tl_log.h"
#include "lib_tl_config.h"
#include "tl_main.h"

static int tl_program_send_cmd(tl_com_t *pPort, char *szCommand, uint8_t iAddress)
{
	if(TL_COM_IDLE != pPort->iLineSate)
		return ERROR;
	strncpy(pPort->arrSend, szCommand, TL_MAX_PACKET_SERIAL_SIZE);
	pPort->arrSend[TL_MAX_PACKET_SERIAL_SIZE-1] = 0;
	return tl_com_send_session(pPort, iAddress, TL_COM_LOCAL_VERIFY);
}

static int32_t tl_program_delay(thread_t *pThread)
{
 	tl_program_t *pProgram = (tl_program_t*)pThread->pArg;
	pProgram->iCurrentStep++;
	 if(pProgram->iCount == pProgram->iCurrentStep)
	 {
		pProgram->iCurrentStep = pProgram->iLoopStep;	
		pProgram->iLoopCount++;
	 }		
	pProgram->pDelayThread = NULL;
	tl_program_step(pProgram);
	return 1;
}

int tl_program_step(tl_program_t *pProgram)
{
	uint8_t iAddress;
	struct timeval stDelay;
	tl_global_config_t *pGlobalDb = tl_get_global_dbase();
	tl_global_t *pGlobal = tl_get_global();	
	
	if(NULL != pProgram->pDelayThread || TL_PROGRAM_RUNNING != pProgram->iState)
		return ERROR;
	while(TL_INSTR_CMD == pProgram->arrInstructions[pProgram->iCurrentStep].iType)
	{
		iAddress = pProgram->arrInstructions[pProgram->iCurrentStep].u.stCommand.iDeviceId;
		if(NULL != pGlobalDb->arrDevices[iAddress] && TL_MODULE_ERR_OK == pGlobalDb->arrDevices[iAddress]->iStatus)
		{
			tl_program_send_cmd(&(pGlobalDb->stPort), pProgram->arrInstructions[pProgram->iCurrentStep].u.stCommand.arrCommand, iAddress);
			tl_update_controler_state(&(pProgram->arrInstructions[pProgram->iCurrentStep]));			
		}
		pProgram->iCurrentStep++;
		if(pProgram->iCount == pProgram->iCurrentStep)
		{
			pProgram->iCurrentStep = pProgram->iLoopStep;
			pProgram->iLoopCount++;
		}
	}
	if(TL_INSTR_DELAY == pProgram->arrInstructions[pProgram->iCurrentStep].iType)
	{
		stDelay.tv_sec = pProgram->arrInstructions[pProgram->iCurrentStep].u.iDelay / 1000;
		stDelay.tv_usec = 1000 * (pProgram->arrInstructions[pProgram->iCurrentStep].u.iDelay % 1000);	
		pProgram->pDelayThread = thread_timer_add(&(pGlobal->stMaintThread), tl_program_delay, pProgram, &stDelay);
	}
	return OK;
}

void tl_program_free(tl_program_t **ppProgram)
{
	if(NULL == ppProgram || NULL == (*ppProgram))
		return;
		
	if((*ppProgram)->pDelayThread)
		thread_cancel((*ppProgram)->pDelayThread);
	if((*ppProgram)->szProgramFile)
		free((*ppProgram)->szProgramFile);
	if((*ppProgram)->arrInstructions)
		free((*ppProgram)->arrInstructions);
	free((*ppProgram));
	(*ppProgram) = NULL;
}

int tl_program_stop(tl_program_t *pProgram)
{
	if(NULL == pProgram)
		return ERROR;
	pProgram->iState = TL_PROGRAM_PAUSE;
	if(pProgram->pDelayThread)
		thread_cancel(pProgram->pDelayThread);
	pProgram->pDelayThread = NULL;
	return OK;	
}

int tl_program_reset(tl_program_t *pProgram)
{
	if(NULL == pProgram)
		return ERROR;
	pProgram->iState = TL_PROGRAM_RUNNING;
	pProgram->iCurrentStep = 0;
	pProgram->iLoopCount = 0;
	return tl_program_step(pProgram);
}

int tl_program_change(tl_api_program_t *pApiProgram)
{
	tl_program_t *pNewProgram=NULL;
	tl_global_t *pGlobal = tl_get_global();
	
	if(NULL == pApiProgram)
		return ERROR;
	if(OK != tl_parse_prog_file(pApiProgram->szProgName, &pNewProgram) || NULL == pNewProgram)
		return ERROR;
	pNewProgram->iLoadMode = pApiProgram->iType;
	switch(pApiProgram->iType)
	{
		case TL_PROGRAM_REPLACE:
			tl_program_free(&(pGlobal->pCurrentProgram));
			pGlobal->pCurrentProgram = pNewProgram;
			if(0 == pGlobal->iCritError)
				tl_program_reset(pGlobal->pCurrentProgram);
			break;
		case TL_PROGRAM_INTERUPT:
		default:
			tl_program_free(&pNewProgram);
			return ERROR;
	}
	return OK;
}



