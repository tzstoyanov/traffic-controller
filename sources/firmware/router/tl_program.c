#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_serial.h"
#include "tl_program.h"
#include "tl_packets.h"
#include "tl_controlers.h"
#include "tl_config.h"
#include "tl_api.h"

static int tl_program_send_cmd(tl_global_t *pGlobal, char *szCommand, uint8_t iAddress)
{
	if(TL_COM_IDLE != pGlobal->stPort.iLineSate)
		return ERROR;
	strncpy(pGlobal->stPort.arrSend, szCommand, TL_MAX_PACKET_SERIAL_SIZE);
	pGlobal->stPort.arrSend[TL_MAX_PACKET_SERIAL_SIZE-1] = 0;
	return tl_com_send_session(&(pGlobal->stPort), iAddress, TL_COM_LOCAL_VERIFY);
}

static int32_t tl_program_delay(thread_t *pThread)
{
 	tl_program_t *pProgram = (tl_program_t*)pThread->pArg;
	pProgram->iCurrentStep++;
	 if(pProgram->iCount == pProgram->iCurrentStep)
		pProgram->iCurrentStep = pProgram->iLoopStep;	
	pProgram->pDelayThread = NULL;
	tl_program_step(pProgram);
	return 1;
}

int tl_program_step(tl_program_t *pProgram)
{
	uint8_t iAddress;
	struct timeval stDelay;
	tl_global_t *pGlobal = tl_get_global();
	
	if(NULL != pProgram->pDelayThread)
		return ERROR;
	while(TL_INSTR_CMD == pProgram->arrInstructions[pProgram->iCurrentStep].iType)
	{
		iAddress = pProgram->arrInstructions[pProgram->iCurrentStep].u.stCommand.iDeviceId;
		if(NULL != pGlobal->arrDevices[iAddress] && TL_MODULE_ERR_OK == pGlobal->arrDevices[iAddress]->iStatus)
		{
			tl_program_send_cmd(pGlobal, pProgram->arrInstructions[pProgram->iCurrentStep].u.stCommand.arrCommand, iAddress);
			tl_update_controler_state(&(pProgram->arrInstructions[pProgram->iCurrentStep]));			
		}
		pProgram->iCurrentStep++;
		if(pProgram->iCount == pProgram->iCurrentStep)
			pProgram->iCurrentStep = pProgram->iLoopStep;
	}
	if(TL_INSTR_DELAY == pProgram->arrInstructions[pProgram->iCurrentStep].iType)
	{
		stDelay.tv_sec = pProgram->arrInstructions[pProgram->iCurrentStep].u.iDelay / 1000;
		stDelay.tv_usec = 1000 * (pProgram->arrInstructions[pProgram->iCurrentStep].u.iDelay % 1000);	
		pProgram->pDelayThread = thread_timer_add(&(pGlobal->stMaintThread), tl_program_delay, pProgram, &stDelay);
	}
	return OK;
}


