#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_packets_api.h"
#include "tl_serial_api.h"
#include "tl_program_api.h"
#include "tl_controlers_api.h"
#include "lib_tl_log.h"
#include "lib_tl_config.h"
#include "lib_tl_com.h"
#include "lib_tl_packets.h"
#include "sim_tl_main.h"

int tl_controler_config_tlv(tl_controler_t *pControler, uint8_t iCfgId, uint8_t iCfgLen, uint8_t *pData)
{
	uint8_t i=0, iCfgOk;
	tl_sim_controler_t *pSim=NULL;
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();
	
	if(NULL == pControler)
		return ERROR;	
	pSim = (tl_sim_controler_t *)pControler->pData;	

	switch(iCfgId)
	{
		case TL_CFGID_QINTERVAL:
			if(iCfgLen < 1)
				return ERROR;
			pSim->iWdInterval = pData[0];
			pSim->iWdInterval *= 10;			
			break;
		case TL_CFGID_DEADCOUNT:
			if(iCfgLen < 1)
				return ERROR;
			pSim->iDeadCount = pData[0];			
			break;
		case TL_CFGID_SLAVES:
			if(TL_MODULE_GATE != pControler->iType)
				return ERROR;
			while(iCfgLen)
			{
				pSim->arrSlaves[i] = pData[i];
				i++;
				iCfgLen--;
			}
			break;
		default:
			return ERROR;
	}
	iCfgOk = 1;
	if(pSim->iWdInterval != pDbGlobal->iWdInterval)
		iCfgOk=0;
	if(pSim->iDeadCount != pDbGlobal->iDeadCount)
		iCfgOk=0;
	if(	TL_MODULE_GATE == pControler->iType 
		&& 0 == pSim->arrSlaves[0])
		iCfgOk=0;
	if(iCfgOk)
		pControler->iStatus = TL_MODULE_ERR_OK;
	return OK;		
}

int tl_controler_config(tl_controler_t *pControler, t_can_packet_t *pPacket)
{
	uint8_t iCfgId, iCfgLen;
	uint8_t iLen = pPacket->iLen, i=0, j;
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();	
		
	while(iLen)
	{
		iCfgId = ((pPacket->arrData[i] & 0xF0) >> 4);
		iCfgLen = (pPacket->arrData[i] & 0x0F);
		i++;
		iLen--;
		if(iCfgLen <= iLen)
		{
			if(NULL == pControler)
			{
				for(i=1;j<TL_MAX_CONTROLERS; j++)
				{
					if(NULL == pDbGlobal->arrDevices[j])
						break;
					tl_controler_config_tlv(pDbGlobal->arrDevices[j], iCfgId, iCfgLen, &(pPacket->arrData[i]));		
				}	
			}
			else	
				tl_controler_config_tlv(pControler, iCfgId, iCfgLen, &(pPacket->arrData[i]));
		}
		iLen -= iCfgLen;
		i+=iCfgLen;	
	}
	return OK;
}

int sim_set_timer_state(tl_controler_timer_t *pTimer, t_can_packet_t *pPacket)
{
	uint8_t iDataLen;
	uint8_t iColor = (pPacket->arrData[0] & 0x18) >> 3;
	uint8_t iAction = (pPacket->arrData[0] & 0x07);
	struct timeval stNow={0,0};

	sim_get_time_rel(&stNow);

	switch(iAction)
	{
		case TL_TIMER_ACT_COUNTDOWN:
			iDataLen = 1;
			break;
		case TL_TIMER_ACT_STATIC:
			iDataLen = 2;
			break;
		default:
			return ERROR;		 
	}	
	
	if(pPacket->iLen < iDataLen+1)
		return ERROR;
		
	pTimer->stCurrentState.iAction = iAction;		
	pTimer->stCurrentState.iColor = iColor;							
	pTimer->stCurrentState.iStateLen = iDataLen;
	while(iDataLen > 0)
	{
		pTimer->stCurrentState.arrState[iDataLen-1] = pPacket->arrData[iDataLen];
		iDataLen--;
	}	
	
	tl_log_info("%s state changed to [%s] in %d.%d",
		pTimer->stHead.szName , tl_timerstate2string(&(pTimer->stCurrentState)),
		stNow.tv_sec, stNow.tv_usec );
	
	return OK;	
}

int sim_set_lamp_state(tl_controler_lamp_t *pLamp, t_can_packet_t *pPacket)
{
	struct timeval stNow={0,0};

	sim_get_time_rel(&stNow);

	pLamp->stCurrentState.iState = pPacket->arrData[0] & 0x0F;
	tl_log_info("%s state changed to [%s] in %d.%d",
		pLamp->stHead.szName , tl_lampstate2string(&(pLamp->stCurrentState)),
		stNow.tv_sec, stNow.tv_usec );
	return OK;
}

int sim_tl_controler_state(tl_controler_t *pControler, t_can_packet_t *pPacket)
{
	uint8_t iPktType;
	if(NULL == pControler || pPacket->iLen < 1)
		return ERROR;
	iPktType = (pPacket->arrData[0] & 0xE0) >> 5;	
	if(iPktType != pControler->iType)
		return ERROR;

	switch(pControler->iType)
	{
		case TL_MODULE_LAMP: 
			return sim_set_lamp_state((tl_controler_lamp_t*)pControler, pPacket);
		case TL_MODULE_TIMER:
			return sim_set_timer_state((tl_controler_timer_t*)pControler, pPacket);
		default:
			break;		
	}	
	return ERROR;
}

int sim_tl_controler(uint8_t iAddress, t_can_packet_t *pPacket)
{
	tl_controler_t *pControler = NULL;
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();

	if(iAddress < TL_MAX_CONTROLERS)
	{	
		if(	NULL == pDbGlobal->arrDevices[iAddress] || 
			NULL == pDbGlobal->arrDevices[iAddress]->pData)
			return ERROR;
		pControler = pDbGlobal->arrDevices[iAddress];
		if(pControler->iWdMiss)
		{
			if(pControler->iWdMiss >= pDbGlobal->iDeadCount)
				tl_log_info("Reset WD counter for %s", pControler->szName);
			pControler->iWdMiss = 0;
		}
	}		
	switch(pPacket->iCommand)
	{
		case TL_CMD_CONFIG:
			tl_controler_config(pControler, pPacket);
			break;
		case TL_CMD_STATE:
			sim_tl_controler_state(pControler, pPacket);
			break;
		case TL_CMD_WD_REQUEST:
			tl_reply_status(pControler, pPacket);
			break;
		case TL_CMD_WD_RESPONCE:
		case TL_CMD_WD_RESPONCE_F:
		default:
			tl_log_error("Received unexpected packet from %d: %d", iAddress, pPacket->iCommand);
			break;
	}
	return OK;
}

tl_controler_t *sim_tl_NewControler(uint8_t iType)
{
	tl_controler_t *pControler = NULL;
	tl_sim_controler_t *pSim = calloc(1, sizeof(tl_sim_controler_t));
	if(NULL == pSim)
		return NULL;
	switch(iType)
	{
		case TL_MODULE_MASTER:
			pControler = calloc(1, sizeof(tl_controler_t));
			pControler->iStatus = TL_MODULE_ERR_INIT;
			pControler->fPktProcess = sim_tl_controler;
			pControler->iCritical = 1;
			break;
		case TL_MODULE_GATE:
			pControler = calloc(1, sizeof(tl_controler_t));
			pControler->iStatus = TL_MODULE_ERR_INIT;
			pControler->fPktProcess = sim_tl_controler;
			pControler->iCritical = 0;
			break;
		case TL_MODULE_LAMP:
			pControler = calloc(1, sizeof(tl_controler_lamp_t));				
			pControler->iStatus = TL_MODULE_ERR_INIT;
			pControler->fPktProcess = sim_tl_controler;
			pControler->iCritical = 1;
			break;
		case TL_MODULE_TIMER:
			pControler = calloc(1, sizeof(tl_controler_timer_t));		
			pControler->iStatus = TL_MODULE_ERR_INIT;
			pControler->fPktProcess = sim_tl_controler;
			pControler->iCritical = 0;
			break;
		default:
			tl_log_error("Uknown controler type %d", iType);
			free(pSim);
			return NULL;
	}
	if(NULL != pControler)
	{
		pControler->iType = iType;
		pControler->pData = pSim;
	}
	return pControler;	
}


