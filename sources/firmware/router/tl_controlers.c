#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_api.h"
#include "tl_serial.h"
#include "tl_program.h"
#include "tl_packets.h"
#include "tl_controlers.h"
#include "tl_config.h"
#include "tl_log.h"

int tl_controler_check_common(uint8_t *arrStatus)
{
	tl_global_t *pGlobal = tl_get_global();
	uint8_t iAddress = arrStatus[0];
	uint8_t iPktType = (arrStatus[1] & 0xE0) >> 5;
	uint8_t iState = (arrStatus[1] & 10) >> 4;
	uint8_t iStatus = (arrStatus[1] & 0x0F);
	if(NULL == pGlobal->arrDevices[iAddress])
		return ERROR;
	if(pGlobal->arrDevices[iAddress]->iType != iPktType)
		return ERROR;
	if(TL_MODULE_STATE_OK == iState)
	{
		pGlobal->arrDevices[iAddress]->iStatus = TL_MODULE_ERR_OK;
		if(TL_MODULE_LAMP == iPktType)
		{
			if(((tl_controler_lamp_t*)(pGlobal->arrDevices[iAddress]))->stCurrentState.iState != iStatus)
			{
				pGlobal->arrDevices[iAddress]->iStatus = TL_MODULE_ERR_MISMATCH;
				tl_log_error("State mismatch of %s (%d): 0x%X != 0x%X !", 
						pGlobal->arrDevices[iAddress]->szName, iAddress, 
						((tl_controler_lamp_t*)(pGlobal->arrDevices[iAddress]))->stCurrentState.iState,
						iStatus );
				return ERROR;
			}
		}
		return OK;
	}
	pGlobal->arrDevices[iAddress]->iStatus = iStatus;
	
	switch(iStatus)
	{
		case TL_MODULE_ERR_OK:
			return OK;
		case TL_MODULE_ERR_INIT:
			return OK;
		case TL_MODULE_ERR_HW:
			break;
		case TL_MODULE_ERR_COMM:
			break;
		case TL_MODULE_ERR_SLAVE:
			break;
		case TL_MODULE_ERR_TIMEOUT:
			break;
		default:
			break;
	}
	
	tl_log_error("Error of %s (%d): 0x%X", 
			pGlobal->arrDevices[iAddress]->szName, iAddress, iStatus );

	return ERROR;
}

int tl_controler_check_module(uint8_t iAddress, t_can_packet_t *pPacket)
{
	if(pPacket->iLen < 2)
		return ERROR;
	return tl_controler_check_common(pPacket->arrData);
}

int tl_controler_check_gate(uint8_t iAddress, t_can_packet_t *pPacket)
{
	uint8_t i, iSize = pPacket->iLen;
	if(pPacket->iLen < 2)
		return ERROR;
	if(OK != tl_controler_check_common(pPacket->arrData))
		return ERROR;
	i = 2;
	iSize -= 2;
	while(iSize >=2)
	{
		tl_controler_check_common(&(pPacket->arrData[i]));
		iSize -= 2;
		i += 2;
	}
	return OK;
}


tl_controler_t *tl_NewControler(uint8_t iType)
{
	tl_controler_t *pControler = NULL;
	switch(iType)
	{
		case TL_MODULE_MASTER:
			pControler = calloc(1, sizeof(tl_controler_t));
			break;
		case TL_MODULE_GATE:
			pControler = calloc(1, sizeof(tl_controler_t));
			pControler->fVerifyState = tl_controler_check_gate;
			break;
		case TL_MODULE_LAMP:
			pControler = calloc(1, sizeof(tl_controler_lamp_t));				
			pControler->fVerifyState = tl_controler_check_module;
			break;
		case TL_MODULE_TIMER:
			pControler = calloc(1, sizeof(tl_controler_timer_t));		
			pControler->fVerifyState = tl_controler_check_module;
			break;
		default:
			tl_log_error("Uknown controler type %d", iType);
			return NULL;
	}
	if(NULL != pControler)
	{
		pControler->iType = iType;
		pControler->iStatus = TL_MODULE_ERR_INIT;
	}
	return pControler;	
}

tl_controler_t *tl_get_controler_byname(char *szName)
{
	int i;
	tl_global_t *pGlobal = tl_get_global();
	
	for(i=0; i<TL_MAX_CONTROLERS; i++)
	{
	 if(NULL == pGlobal->arrDevices[i])
		return NULL;
	 if(0 == strcmp(szName, pGlobal->arrDevices[i]->szName))	
		return pGlobal->arrDevices[i];
	}	
	return NULL;
}

int tl_send_gate_slaves(uint8_t iGate)
{
	int i, j=0;
	uint8_t  arrSlaves[7];	
	tl_global_t *pGlobal = tl_get_global();
	
	if(NULL == pGlobal->arrDevices[iGate] || TL_MODULE_GATE != pGlobal->arrDevices[iGate]->iType)	
		return ERROR;
	if(TL_COM_IDLE != pGlobal->stPort.iLineSate)
		return ERROR;		
	for(i = iGate+1; i<TL_MAX_CONTROLERS; i++)
	{
		if(iGate == pGlobal->arrDevices[i]->iGateId && TL_MODULE_GATE != pGlobal->arrDevices[i]->iType)
			arrSlaves[j++] = i;
		else
			break;
		if(j >= 7)
		{
			tl_cfg_command_set(pGlobal->stPort.arrSend, iGate, TL_CFGID_SLAVES, arrSlaves, 7);
			if(TL_IS_DEBUG(TX_CFG))
			{
				tl_log_info("Send config %d Slaves to %s (%d)", 
							j, pGlobal->arrDevices[iGate]->szName, iGate);
				if(TL_IS_DEBUG(PKT_DUMP))
					tl_log_info("Send packet [%s]", pGlobal->stPort.arrSend);
			}	
			tl_com_send_session(&(pGlobal->stPort), iGate, TL_COM_LOCAL_VERIFY);
			j = 0;
		}
	}
	if(j)
	{
		tl_cfg_command_set(pGlobal->stPort.arrSend, iGate, TL_CFGID_SLAVES, arrSlaves, j<=7?j:7);
		if(TL_IS_DEBUG(TX_CFG))
		{
			tl_log_info("Send config %d Slaves to %s (%d)", 
						j, pGlobal->arrDevices[iGate]->szName, iGate);
			if(TL_IS_DEBUG(PKT_DUMP))
				tl_log_info("Send packet [%s]", pGlobal->stPort.arrSend);
		}	
		
		tl_com_send_session(&(pGlobal->stPort), iGate, TL_COM_LOCAL_VERIFY);
	}	
	return OK;
}

int tl_send_wdinterval()
{
	uint8_t iData;
	tl_global_t *pGlobal = tl_get_global();
	if(TL_COM_IDLE != pGlobal->stPort.iLineSate)
		return ERROR;
	iData = pGlobal->iWdInterval/10;
	tl_cfg_command_set(pGlobal->stPort.arrSend, TL_ADDRESS_BCAST, TL_CFGID_QINTERVAL, &iData, 1);
	if(TL_IS_DEBUG(TX_CFG))
	{
		tl_log_info("Send config WD interval %d to all", iData);
		if(TL_IS_DEBUG(PKT_DUMP))
			tl_log_info("Send packet [%s]", pGlobal->stPort.arrSend);
	}	
	
	return tl_com_send_session(&(pGlobal->stPort), TL_ADDRESS_BCAST, TL_COM_LOCAL_VERIFY);
}

int tl_update_controler_state(tl_instruction_t *pCommand)
{
	uint8_t iAddress;
	void *pDbState, *pCmdState;
	uint8_t iSize;
	tl_global_t *pGlobal = tl_get_global();

	iAddress = 	pCommand->u.stCommand.iDeviceId;	
	if(TL_INSTR_CMD !=pCommand->iType || NULL == pGlobal->arrDevices[iAddress])
		return ERROR;
	switch(pGlobal->arrDevices[iAddress]->iType)
	{
		case TL_MODULE_LAMP:
			pDbState = &(((tl_controler_lamp_t*)&(pGlobal->arrDevices[iAddress]))->stCurrentState);
			pCmdState = &(pCommand->u.stCommand.state.stLamp);
			iSize = sizeof(tl_lamp_state_t);
			if(TL_IS_DEBUG(LAMP))
				tl_log_info("Controler %s (%d) at address %d changed its state to 0x%X",
						pGlobal->arrDevices[iAddress]->szName, pGlobal->arrDevices[iAddress]->iId,
						pCommand->u.stCommand.state.stLamp.iState);
			break;
		case TL_MODULE_TIMER:
			pDbState = &(((tl_controler_timer_t*)&(pGlobal->arrDevices[iAddress]))->stCurrentState);
			pCmdState = &(pCommand->u.stCommand.state.stTimer);
			iSize = sizeof(tl_timer_state_t);
			if(TL_IS_DEBUG(TIMER))
				tl_log_info("Controler %s (%d) at address %d changed its state to Action %d, color %d, state 0x%X 0x%X 0x%X",
						pGlobal->arrDevices[iAddress]->szName, iAddress,
						pCommand->u.stCommand.state.stTimer.iCurrentAction, 
						pCommand->u.stCommand.state.stTimer.iCurrentColor,
						pCommand->u.stCommand.state.stTimer.arrCurrentState[0],
						pCommand->u.stCommand.state.stTimer.arrCurrentState[1],
						pCommand->u.stCommand.state.stTimer.arrCurrentState[2]);
			
			break;
		default:
			return ERROR;
	}
	memcpy(pDbState, pCmdState, iSize);
	return OK;
}

int tl_ask_controler_state(uint8_t iAddress)
{
	tl_global_t *pGlobal = tl_get_global();
	if(TL_COM_IDLE != pGlobal->stPort.iLineSate)
		return ERROR;
	if(OK != tl_wdrequest_command_set(pGlobal->stPort.arrSend, iAddress, pGlobal->arrDevices[0]->iId))
		return ERROR;
	if(TL_IS_DEBUG(TX_RQST))
	{
		if( NULL != pGlobal->arrDevices[iAddress] )
			tl_log_info("Send WD request to %s at address %d", 
					pGlobal->arrDevices[iAddress]->szName, iAddress);
		if(TL_IS_DEBUG(PKT_DUMP))
			tl_log_info("Send packet [%s]", pGlobal->stPort.arrSend);
	}	
 	return tl_com_send_session(&(pGlobal->stPort), iAddress, TL_COM_LOCAL_VERIFY_WAIT);
}

int tl_check_controlers()
{
	int i, iOk=0, iFail=0;
	tl_global_t *pGlobal = tl_get_global();
		
	for(i=0; i<TL_MAX_CONTROLERS; i++)
	{
		if(NULL == pGlobal->arrDevices[i])
			break;
		tl_ask_controler_state(i);
		if(TL_MODULE_ERR_OK == pGlobal->arrDevices[i]->iStatus)
			iOk++;
		else
			iFail++;
		if(	(TL_MODULE_GATE == pGlobal->arrDevices[i]->iType && TL_IS_DEBUG(GATE)) ||
			(TL_MODULE_LAMP == pGlobal->arrDevices[i]->iType && TL_IS_DEBUG(LAMP)) ||
			(TL_MODULE_TIMER == pGlobal->arrDevices[i]->iType && TL_IS_DEBUG(TIMER)) )
		{
			tl_log_info("Controler %s (%d) at address %d status is 0x%X",  
				pGlobal->arrDevices[i]->szName, pGlobal->arrDevices[i]->iType,
				pGlobal->arrDevices[i]->iId,  pGlobal->arrDevices[i]->iStatus);
		}
	}

	if(TL_IS_DEBUG(INIT))
		tl_log_info("Checked controlers state, total %d: %d OK, %d Fail", i, iOk, iFail);	
	
	return OK;	
}

int tl_controler_timeout(uint8_t iAddress)
{
	tl_global_t *pGlobal = tl_get_global();
	
	if(iAddress < TL_MAX_CONTROLERS && NULL != pGlobal->arrDevices[iAddress])
	{
		pGlobal->arrDevices[iAddress]->iStatus = TL_MODULE_ERR_TIMEOUT;
		tl_log_error("Timeout with controler %s (%d) at address %d",
			pGlobal->arrDevices[iAddress]->szName, 
			pGlobal->arrDevices[iAddress]->iType, iAddress);		
	}	

	return OK;
}
