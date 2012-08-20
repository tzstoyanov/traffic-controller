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
#include "lib_tl_packets.h"
#include "tl_main.h"

int tl_controler_check_common(uint8_t *arrStatus)
{
	tl_global_config_t *pGlobal = tl_get_global_dbase();
	uint8_t iAddress = arrStatus[0];
	uint8_t iPktType = (arrStatus[1] & 0xE0) >> 5;
	uint8_t iState = (arrStatus[1] & 0x10) >> 4;
	uint8_t iStatus = (arrStatus[1] & 0x0F);
	uint8_t iNewStatus;
	
	if(NULL == pGlobal->arrDevices[iAddress])
		return ERROR;

	if(pGlobal->arrDevices[iAddress]->iType != iPktType)
		return ERROR;
		
	if(pGlobal->arrDevices[iAddress]->iWdMiss)
	{
		if(pGlobal->arrDevices[iAddress]->iWdMiss >= pGlobal->iDeadCount)
			tl_log_info("Reset WD counter for %s", pGlobal->arrDevices[iAddress]->szName);
		pGlobal->arrDevices[iAddress]->iWdMiss = 0;
	}

	if(TL_MODULE_STATE_OK == iState)
	{
		iNewStatus = TL_MODULE_STATE_OK;
		if(TL_MODULE_LAMP == iPktType)
		{
			if(((tl_controler_lamp_t*)(pGlobal->arrDevices[iAddress]))->stCurrentState.iState != iStatus)
			{
				iNewStatus = TL_MODULE_STATE_MISMATCH;
				if(TL_MODULE_STATE_MISMATCH != pGlobal->arrDevices[iAddress]->iStatus)
					tl_log_error("State mismatch of %s (%d): 0x%X != 0x%X !", 
						pGlobal->arrDevices[iAddress]->szName, iAddress, 
						((tl_controler_lamp_t*)(pGlobal->arrDevices[iAddress]))->stCurrentState.iState,
						iStatus );
				pGlobal->arrDevices[iAddress]->iErrorCount++;		
				if(	pGlobal->arrDevices[iAddress]->iErrorCount < pGlobal->iDeadCount && 
					pGlobal->arrDevices[iAddress]->bLastCommandValid)
					tl_program_send_cmd(&(pGlobal->stPort), pGlobal->arrDevices[iAddress]->arrLastCommand, iAddress);
			}else
				pGlobal->arrDevices[iAddress]->iErrorCount = 0;
		}else
			pGlobal->arrDevices[iAddress]->iErrorCount = 0;
	}
	else
	{
		pGlobal->arrDevices[iAddress]->iErrorCount++;
		iNewStatus = iStatus;
	}

	if(iNewStatus != pGlobal->arrDevices[iAddress]->iStatus)
	{
		if(TL_MODULE_STATE_OK == iNewStatus)
			tl_log_info("Module %s is back to normal operation", pGlobal->arrDevices[iAddress]->szName);
		else
			tl_log_error("Module %s at adress %d enters in state %s", 
				pGlobal->arrDevices[iAddress]->szName, iAddress, tl_status2string(iNewStatus));
	}	
	
	pGlobal->arrDevices[iAddress]->iStatus = iNewStatus;
	tl_check_system_status();

	switch(iNewStatus)
	{
		case TL_MODULE_STATE_OK:
			pGlobal->arrDevices[iAddress]->iSendConfig=0;
			return OK;
		case TL_MODULE_STATE_INIT:
			pGlobal->arrDevices[iAddress]->iSendConfig++;
			return OK;
		case TL_MODULE_STATE_HW_ERR:
			break;
		case TL_MODULE_STATE_HW_WARN:
			break;
		case TL_MODULE_STATE_COMM_ERR:
			break;
		case TL_MODULE_STATE_TIMEOUT:
			break;
		case TL_MODULE_STATE_MISMATCH:
			break;	
		default:
			break;
	}
	
	if(TL_MODULE_STATE_OK == iNewStatus)
		return OK;
	return ERROR;
}

int tl_controler_check_module(uint8_t iAddress, t_can_packet_t *pPacket)
{

	if(	TL_CMD_WD_RESPONCE != pPacket->iCommand && 
		TL_CMD_WD_RESPONCE_F != pPacket->iCommand)
		return OK;
	if(pPacket->iLen < 2)
		return ERROR;

	return tl_controler_check_common(pPacket->arrData);
}

int tl_controler_check_gate(uint8_t iAddress, t_can_packet_t *pPacket)
{
	uint8_t i, iSize = pPacket->iLen;
	
	if(	TL_CMD_WD_RESPONCE != pPacket->iCommand && 
		TL_CMD_WD_RESPONCE_F != pPacket->iCommand)
		return OK;

	if(pPacket->iLen < 2)
		return ERROR;
		
	tl_controler_check_common(pPacket->arrData);
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
			pControler->iStatus = TL_MODULE_STATE_OK;
			pControler->iCritical = 1;
			break;
		case TL_MODULE_GATE:
			pControler = calloc(1, sizeof(tl_controler_t));
			pControler->fPktProcess = tl_controler_check_gate;
			pControler->iStatus = TL_MODULE_STATE_INIT;
			pControler->iCritical = 0;
			break;
		case TL_MODULE_LAMP:
			pControler = calloc(1, sizeof(tl_controler_lamp_t));				
			pControler->fPktProcess = tl_controler_check_module;
			pControler->iStatus = TL_MODULE_STATE_INIT;
			pControler->iCritical = 1;
			break;
		case TL_MODULE_TIMER:
			pControler = calloc(1, sizeof(tl_controler_timer_t));		
			pControler->fPktProcess = tl_controler_check_module;
			pControler->iStatus = TL_MODULE_STATE_INIT;
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

tl_controler_t *tl_get_controler_byname(char *szName)
{
	int i;
	tl_global_config_t *pGlobal = tl_get_global_dbase();
	
	for(i=1; i<TL_MAX_CONTROLERS; i++)
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
	tl_global_config_t *pGlobal = tl_get_global_dbase();
	
	if(NULL == pGlobal->arrDevices[iGate] || TL_MODULE_GATE != pGlobal->arrDevices[iGate]->iType)	
		return ERROR;
	if(TL_COM_IDLE != pGlobal->stPort.iLineState)
		return ERROR;
	for(i = iGate+1; i<TL_MAX_CONTROLERS; i++)
	{
		if(NULL == pGlobal->arrDevices[i])
			break;
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
			pGlobal->stPort.iSendSize = strlen(pGlobal->stPort.arrSend);
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
		pGlobal->stPort.iSendSize = strlen(pGlobal->stPort.arrSend);
		tl_com_send_session(&(pGlobal->stPort), iGate, TL_COM_LOCAL_VERIFY);
	}	
	return OK;
}

int tl_send_wdinterval(uint8_t iAddress)
{
	uint8_t iData;
	tl_global_config_t *pGlobal = tl_get_global_dbase();
	if(TL_COM_IDLE != pGlobal->stPort.iLineState)
		return ERROR;
	iData = pGlobal->iWdInterval/10;
	tl_cfg_command_set(pGlobal->stPort.arrSend, iAddress, TL_CFGID_QINTERVAL, &iData, 1);
	if(TL_IS_DEBUG(TX_CFG))
	{
		tl_log_info("Send config WD interval %d to %d", iData, iAddress);
		if(TL_IS_DEBUG(PKT_DUMP))
			tl_log_info("Send packet [%s]", pGlobal->stPort.arrSend);
	}	
	pGlobal->stPort.iSendSize = strlen(pGlobal->stPort.arrSend);
	return tl_com_send_session(&(pGlobal->stPort), iAddress, TL_COM_LOCAL_VERIFY);
}

int tl_send_deadcount(uint8_t iAddress)
{
	uint8_t iData;
	tl_global_config_t *pGlobal = tl_get_global_dbase();
	if(TL_COM_IDLE != pGlobal->stPort.iLineState)
		return ERROR;
	iData = pGlobal->iDeadCount;
	tl_cfg_command_set(pGlobal->stPort.arrSend, iAddress, TL_CFGID_DEADCOUNT, &iData, 1);
	if(TL_IS_DEBUG(TX_CFG))
	{
		tl_log_info("Send config Dead Count %d to %d", iData, iAddress);
		if(TL_IS_DEBUG(PKT_DUMP))
			tl_log_info("Send packet [%s]", pGlobal->stPort.arrSend);
	}	
	pGlobal->stPort.iSendSize = strlen(pGlobal->stPort.arrSend);
	return tl_com_send_session(&(pGlobal->stPort), iAddress, TL_COM_LOCAL_VERIFY);
}

int tl_send_comtimeout(uint8_t iAddress)
{
	uint8_t iData;
	tl_global_config_t *pGlobal = tl_get_global_dbase();
	if(TL_COM_IDLE != pGlobal->stPort.iLineState)
		return ERROR;
	iData = pGlobal->stPort.iTimeout;
	tl_cfg_command_set(pGlobal->stPort.arrSend, iAddress, TL_CFGID_COMTIMEOUT, &iData, 1);
	if(TL_IS_DEBUG(TX_CFG))
	{
		tl_log_info("Send config Timeout %d to %d", iData, iAddress);
		if(TL_IS_DEBUG(PKT_DUMP))
			tl_log_info("Send packet [%s]", pGlobal->stPort.arrSend);
	}	
	pGlobal->stPort.iSendSize = strlen(pGlobal->stPort.arrSend);
	return tl_com_send_session(&(pGlobal->stPort), iAddress, TL_COM_LOCAL_VERIFY);
}


int tl_send_controlers_config(uint8_t iAddress)
{
 uint8_t i;
 tl_global_config_t *pGlobal = tl_get_global_dbase();
 
	tl_send_wdinterval(iAddress);
	tl_send_deadcount(iAddress);
	tl_send_comtimeout(iAddress);
	if(TL_ADDRESS_BCAST == iAddress)
	{
		for(i=1; i<TL_MAX_CONTROLERS; i++)
		{
			if(NULL == pGlobal->arrDevices[i])
				break;	
			if(TL_MODULE_GATE == pGlobal->arrDevices[i]->iType)
				tl_send_gate_slaves(i);
		}
	}
	else
	{
		if(	NULL != pGlobal->arrDevices[iAddress] &&
			TL_MODULE_GATE == pGlobal->arrDevices[iAddress]->iType)
			tl_send_gate_slaves(iAddress);
	}
	return OK;
}

int tl_update_controler_state(tl_instruction_t *pCommand)
{
	uint8_t iAddress;
	void *pDbState, *pCmdState;
	uint8_t iSize;
	tl_global_config_t *pGlobal = tl_get_global_dbase();

	iAddress = 	pCommand->u.stCommand.iDeviceId;	
	if(TL_INSTR_CMD !=pCommand->iType || NULL == pGlobal->arrDevices[iAddress])
		return ERROR;
	memcpy(pGlobal->arrDevices[iAddress]->arrLastCommand, pCommand->u.stCommand.arrCommand, TL_MAX_PACKET_SERIAL_SIZE);
	pGlobal->arrDevices[iAddress]->bLastCommandValid = 1;
	switch(pGlobal->arrDevices[iAddress]->iType)
	{
		case TL_MODULE_LAMP:
			pDbState = &(((tl_controler_lamp_t*)(pGlobal->arrDevices[iAddress]))->stCurrentState);
			pCmdState = &(pCommand->u.stCommand.state.stLamp);
			iSize = sizeof(tl_lamp_state_t);
			if(TL_IS_DEBUG(LAMP))
				tl_log_info("Controler %s at address %d changed its state to [%s], 0x%02X",
						pGlobal->arrDevices[iAddress]->szName, pGlobal->arrDevices[iAddress]->iId,
						tl_lampstate2string(&(pCommand->u.stCommand.state.stLamp)),
						pCommand->u.stCommand.state.stLamp.iState);
			break;
		case TL_MODULE_TIMER:
			pDbState = &(((tl_controler_timer_t*)(pGlobal->arrDevices[iAddress]))->stCurrentState);
			pCmdState = &(pCommand->u.stCommand.state.stTimer);
			iSize = sizeof(tl_timer_state_t);
			if(TL_IS_DEBUG(TIMER))
				tl_log_info("Controler %s at address %d changed its state to [%s]",
						pGlobal->arrDevices[iAddress]->szName, iAddress,
						tl_timerstate2string(&(pCommand->u.stCommand.state.stTimer)));
			break;
		default:
			return ERROR;
	}
	memcpy(pDbState, pCmdState, iSize);
	
	return OK;
}

int tl_ask_controler_state(uint8_t iAddress)
{
	tl_global_config_t *pGlobal = tl_get_global_dbase();
	if(TL_COM_IDLE != pGlobal->stPort.iLineState)
		return ERROR;
	if(OK != tl_wdrequest_command_set(pGlobal->stPort.arrSend, iAddress, pGlobal->arrDevices[1]->iId))
		return ERROR;
	if(TL_IS_DEBUG(TX_RQST))
	{
		if( NULL != pGlobal->arrDevices[iAddress] )
			tl_log_info("Send WD request to %s at address %d", 
					pGlobal->arrDevices[iAddress]->szName, iAddress);
		if(TL_IS_DEBUG(PKT_DUMP))
			tl_log_info("Send packet [%s]", pGlobal->stPort.arrSend);
	}	
	pGlobal->stPort.iSendSize = strlen(pGlobal->stPort.arrSend);
 	return tl_com_send_session(&(pGlobal->stPort), iAddress, TL_COM_LOCAL_VERIFY_WAIT);
}

int tl_send_failsafe()
{
	tl_global_config_t *pGlobal = tl_get_global_dbase();
	if(TL_COM_IDLE != pGlobal->stPort.iLineState)
		return ERROR;
	if(OK != tl_failsafe_command_set(pGlobal->stPort.arrSend))
		return ERROR;
	if(TL_IS_DEBUG(TX_CMD))	
	{
		tl_log_info("Send FailSafe state packet to all devices");
	}
 	return tl_com_send_session(&(pGlobal->stPort), TL_ADDRESS_BCAST, TL_COM_LOCAL_VERIFY);	
}

int tl_check_controlers(uint8_t *pAll, uint8_t *pOk, uint8_t *pFail)
{
	int i, iOk=0, iFail=0;
	tl_global_config_t *pGlobal = tl_get_global_dbase();
		
	for(i=1; i<TL_MAX_CONTROLERS; i++)
	{
		if(NULL == pGlobal->arrDevices[i])
			break;
		
		if(TL_MODULE_MASTER != pGlobal->arrDevices[i]->iType)
			tl_ask_controler_state(i);
		if(TL_MODULE_STATE_OK == pGlobal->arrDevices[i]->iStatus)
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
		tl_log_info("Checked controlers state, total %d: %d OK, %d Fail", i-1, iOk, iFail);	
	
	return OK;	
}

int tl_controlers_get_state(uint8_t *pAll, uint8_t *pOk, uint8_t *pFail)
{
	int i, iOk=0, iFail=0;
	tl_global_config_t *pGlobal = tl_get_global_dbase();
		
	for(i=1; i<TL_MAX_CONTROLERS; i++)
	{
		if(NULL == pGlobal->arrDevices[i])
			break;

		if(TL_MODULE_STATE_OK == pGlobal->arrDevices[i]->iStatus)
			iOk++;
		else
			iFail++;
	}
	*pAll = i-1;
	*pOk = iOk;
	*pFail = iFail;	
		
	return OK;	
}

int tl_controler_timeout(uint8_t iAddress)
{
	tl_global_config_t *pGlobal = tl_get_global_dbase();
	if(iAddress < TL_MAX_CONTROLERS && NULL != pGlobal->arrDevices[iAddress])
	{
		if(TL_MODULE_STATE_TIMEOUT != pGlobal->arrDevices[iAddress]->iStatus)
		{
			tl_log_error("%s for module %s at adress %d", 
				tl_status2string(TL_MODULE_STATE_TIMEOUT), 
				pGlobal->arrDevices[iAddress]->szName, iAddress);
		}
		pGlobal->arrDevices[iAddress]->iStatus = TL_MODULE_STATE_TIMEOUT;
		pGlobal->arrDevices[iAddress]->iErrorCount++;
		tl_check_system_status();
	}	
	return OK;
}

