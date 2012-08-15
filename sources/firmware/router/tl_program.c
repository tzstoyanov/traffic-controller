#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_packets_api.h"
#include "tl_api.h"
#include "tl_serial.h"
#include "tl_program.h"
#include "tl_controlers.h"
#include "lib_tl_log.h"
#include "lib_tl_config.h"
#include "tl_main.h"

int tl_program_send_cmd(tl_com_t *pPort, char *szCommand, uint8_t iAddress)
{
	int iRet;
	if(TL_COM_IDLE != pPort->iLineState)
		return ERROR;
	strncpy(pPort->arrSend, szCommand, TL_MAX_PACKET_SERIAL_SIZE);
	pPort->arrSend[TL_MAX_PACKET_SERIAL_SIZE-1] = 0;
	pPort->iSendSize = strlen(pPort->arrSend);
	iRet = tl_com_send_session(pPort, iAddress, TL_COM_LOCAL_VERIFY);
	if(TL_IS_DEBUG(TX_CMD))
		tl_log_info("Send Command [%s] to %d, %d bytes", 
				pPort->arrSend, iAddress, pPort->iSendSize);

	return iRet;
}

static int32_t tl_program_delay(thread_t *pThread)
{
 	tl_program_t *pProgram = (tl_program_t*)pThread->pArg;
	pProgram->iCurrentStep++;
	pProgram->pDelayThread = NULL;
	
	if( TL_PROGRAM_EXITING != pProgram->iState && pProgram->iExitStep == pProgram->iLoopEnd)	
	{
		pProgram->iCurrentStep = pProgram->iLoopStep;	
		pProgram->iLoopCount++;
		if(TL_IS_DEBUG(PROGRAM))
			tl_log_info("Program looped at step %d", pProgram->iCurrentStep);				
	}else if(pProgram->iCount == pProgram->iCurrentStep ||
			 (TL_PROGRAM_EXITING == pProgram->iState && 0 == pProgram->iExitStep))
	{
		pProgram->iState = TL_PROGRAM_EXIT;
		if(TL_IS_DEBUG(PROGRAM))
			tl_log_info("Program exit at step %d", pProgram->iCurrentStep);				
		if(pProgram->fExit)
			pProgram->fExit(pProgram->pExitCookie);
		return 1;
	}
		
	tl_program_step(pProgram);
	return 1;
}

void tl_program_invalid()
{
 uint8_t i;	
	tl_global_config_t *pGlobalDb = tl_get_global_dbase();
	for(i = 0; i<TL_MAX_CONTROLERS; i++)	
	{
		if(NULL == pGlobalDb->arrDevices[i])
			break;
		pGlobalDb->arrDevices[i]->bLastCommandValid = 0;
	}
}

int tl_program_step(tl_program_t *pProgram)
{
	uint8_t iAddress;
	struct timeval stDelay;
	tl_global_config_t *pGlobalDb = tl_get_global_dbase();
	tl_global_t *pGlobal = tl_get_global();	
	
	if(	NULL != pProgram->pDelayThread || 
		(TL_PROGRAM_RUNNING != pProgram->iState && TL_PROGRAM_EXITING != pProgram->iState ))
		return ERROR;
	if(TL_PROGRAM_EXITING == pProgram->iState && 0 == pProgram->iExitStep)
	{
			pProgram->iState = TL_PROGRAM_EXIT;
			if(TL_IS_DEBUG(PROGRAM))
				tl_log_info("Program exit at step %d", pProgram->iCurrentStep);			
			if(pProgram->fExit)
				pProgram->fExit(pProgram->pExitCookie);
			return OK;	
	}	
	while(TL_INSTR_CMD == pProgram->arrInstructions[pProgram->iCurrentStep].iType)
	{
		if(TL_IS_DEBUG(PROGRAM))
			tl_log_info("Executing program step %d, command", pProgram->iCurrentStep);	
		iAddress = pProgram->arrInstructions[pProgram->iCurrentStep].u.stCommand.iDeviceId;
		if(NULL != pGlobalDb->arrDevices[iAddress])
		{
			tl_program_send_cmd(&(pGlobalDb->stPort), pProgram->arrInstructions[pProgram->iCurrentStep].u.stCommand.arrCommand, iAddress);
			tl_update_controler_state(&(pProgram->arrInstructions[pProgram->iCurrentStep]));			
		}

		pProgram->iCurrentStep++;
		if( TL_PROGRAM_EXITING != pProgram->iState && pProgram->iExitStep == pProgram->iLoopEnd)
		{
			pProgram->iCurrentStep = pProgram->iLoopStep;
			pProgram->iLoopCount++;
			if(TL_IS_DEBUG(PROGRAM))
				tl_log_info("Program looped at step %d", pProgram->iCurrentStep);			
		}else if(pProgram->iCount == pProgram->iCurrentStep)
		{
			pProgram->iState = TL_PROGRAM_EXIT;
			if(TL_IS_DEBUG(PROGRAM))
				tl_log_info("Program exit at step %d", pProgram->iCurrentStep);			
			if(pProgram->fExit)
				pProgram->fExit(pProgram->pExitCookie);
			return OK;
		}
	}
	if(TL_INSTR_DELAY == pProgram->arrInstructions[pProgram->iCurrentStep].iType)
	{
		if(TL_IS_DEBUG(PROGRAM))
			tl_log_info("Executing program step %d, delay %ds", pProgram->iCurrentStep,
						pProgram->arrInstructions[pProgram->iCurrentStep].u.iDelay);		
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
	tl_program_invalid();
	if((*ppProgram)->pDelayThread)
		thread_cancel((*ppProgram)->pDelayThread);
	if((*ppProgram)->szProgramFile)
		free((*ppProgram)->szProgramFile);
	if((*ppProgram)->arrInstructions)
		free((*ppProgram)->arrInstructions);
	free((*ppProgram));
	(*ppProgram) = NULL;
}

int tl_program_exit(tl_program_t *pProgram, tl_program_exit_callback_t fExit, void *pCookie)
{
	pProgram->fExit = fExit;
	pProgram->pExitCookie = pCookie;

	pProgram->iCurrentStep = pProgram->iExitStep;
	pProgram->iState = TL_PROGRAM_EXITING;
			
	if(TL_IS_DEBUG(PROGRAM))
		tl_log_info("Exit current program");	
	if(NULL == pProgram->pDelayThread)
		tl_program_step(pProgram);	
		
	return OK;	
}

int tl_program_pause(tl_program_t *pProgram)
{
	if(NULL == pProgram)
		return ERROR;
	pProgram->iState = TL_PROGRAM_PAUSE;
	tl_program_invalid();
	if(pProgram->pDelayThread)
		thread_cancel(pProgram->pDelayThread);
	pProgram->pDelayThread = NULL;
	if(TL_IS_DEBUG(PROGRAM))
		tl_log_info("Pause current program");	
	return OK;	
}

int tl_program_reset(tl_program_t *pProgram)
{
	if(NULL == pProgram)
		return ERROR;
	tl_program_invalid();	
	pProgram->iState = TL_PROGRAM_RUNNING;
	pProgram->iCurrentStep = 0;
	pProgram->iLoopCount = 0;
	if(TL_IS_DEBUG(PROGRAM))
		tl_log_info("Reseting current program");
	return tl_program_step(pProgram);
}

void tl_program_replace(void *pCookie)
{
	tl_global_t *pGlobal = tl_get_global();

	tl_program_free(&(pGlobal->pCurrentProgram));
	pGlobal->pCurrentProgram = (tl_program_t*)pCookie;
	if(TL_IS_DEBUG(PROGRAM))
		tl_log_info("Replace current program with %s", pGlobal->pCurrentProgram->szProgramFile);	
	if(0 == pGlobal->iCritError)
		tl_program_reset(pGlobal->pCurrentProgram);	
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
			if(pGlobal->pCurrentProgram)
				tl_program_exit(pGlobal->pCurrentProgram, tl_program_replace, pNewProgram);
			else
				tl_program_replace(pNewProgram);				
			break;
		case TL_PROGRAM_INTERUPT:
		default:
			tl_program_free(&pNewProgram);
			return ERROR;
	}
	return OK;
}

