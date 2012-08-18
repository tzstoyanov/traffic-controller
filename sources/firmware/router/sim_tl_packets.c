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
#include "sim_tl_main.h"

int tl_reply_gate_status(tl_controler_t *pControler, uint8_t iDest)
{
	uint8_t iCount = 0, i, j=0, iCommand;
	uint8_t iReplyStatus, iReplySegments;
	uint8_t arrSlaves[TL_MAX_PACKET_CAN_DATA/2], arrStatus[TL_MAX_PACKET_CAN_DATA/2];
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();			
	tl_sim_controler_t *pSim = NULL;
	
	if(NULL == pControler)
		return ERROR;

	for(i = pControler->iId; i<TL_MAX_CONTROLERS; i++)
	{
		if(NULL == pDbGlobal->arrDevices[i])
			break;
		if(	i == pControler->iId || 
			(pControler->iId == pDbGlobal->arrDevices[i]->iGateId && TL_MODULE_GATE != pDbGlobal->arrDevices[i]->iType))
				iCount++;
		else
				break;
	}
	if(iCount)
	{
		for(i = pControler->iId; i<TL_MAX_CONTROLERS; i++)
		{
			if(NULL == pDbGlobal->arrDevices[i])
				break;
			if( i == pControler->iId || 
				(pControler->iId == pDbGlobal->arrDevices[i]->iGateId && TL_MODULE_GATE != pDbGlobal->arrDevices[i]->iType))
			{	
				arrSlaves[j] = i;
				pSim = (tl_sim_controler_t *)pDbGlobal->arrDevices[i]->pData;
				if(NULL == pSim)
					continue;
				if(pSim->iSimStatus >= 0)
					iReplyStatus = pSim->iSimStatus;
				else
					iReplyStatus = pDbGlobal->arrDevices[i]->iStatus;
				if(TL_MODULE_LAMP == pDbGlobal->arrDevices[i]->iType)
				{	
					if(pSim->iSimSegments >= 0)
						iReplySegments = pSim->iSimSegments;
					else
						iReplySegments = ((tl_controler_lamp_t*)(pDbGlobal->arrDevices[i]))->stCurrentState.iState;
				}	
				if(TL_MODULE_STATE_OK != iReplyStatus)
					arrStatus[j] = 	((pDbGlobal->arrDevices[i]->iType & 0x07) << 5) |
									((TL_MODULE_STATUS_FAIL & 0x01) << 4) |
									(iReplyStatus & 0x0F);
				else if(TL_MODULE_LAMP != pDbGlobal->arrDevices[i]->iType)
					arrStatus[j] = 	((pDbGlobal->arrDevices[i]->iType & 0x07) << 5) |
									((TL_MODULE_STATUS_OK & 0x01) << 4) |
									(iReplyStatus & 0x0F);
				 else
					arrStatus[j] = 	(((pDbGlobal->arrDevices[i]->iType & 0x07) << 5) |
									((TL_MODULE_STATUS_OK & 0x01) << 4) |
									(iReplySegments & 0x0F));
				j++;
				iCount--;
			}
			else
				break;
			if(j == TL_MAX_PACKET_CAN_DATA/2)
			{
				iCommand = TL_CMD_WD_RESPONCE;
				if(iCount)
					iCommand = TL_CMD_WD_RESPONCE_F;
				if(OK==tl_wdresponce_gate_command_set(pDbGlobal->stPort.arrSend, 
							iCommand, iDest, arrSlaves, arrStatus, j))
				{
					pDbGlobal->stPort.iSendSize = strlen(pDbGlobal->stPort.arrSend);			
					tl_com_send(&(pDbGlobal->stPort));						
				}			
				j = 0;
			}
		}
		if(j)
		{
			if(OK==tl_wdresponce_gate_command_set(pDbGlobal->stPort.arrSend, 
						TL_CMD_WD_RESPONCE, iDest, arrSlaves, arrStatus, j))
			{
				pDbGlobal->stPort.iSendSize = strlen(pDbGlobal->stPort.arrSend);			
				tl_com_send(&(pDbGlobal->stPort));						
			}
		}		
	}		
	return OK;
}

int tl_reply_status(tl_controler_t *pControler, t_can_packet_t *pPacket)
{
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();			
	tl_sim_controler_t *pSim = (tl_sim_controler_t *)(pControler->pData);
	tl_controler_lamp_t* pLamp;
	int iLampSegments=-1, iRet;
	
	if(NULL == pControler)
		return ERROR;

	if(1 != pPacket->iLen)
		return ERROR;	
	switch(pControler->iType)
	{
		case TL_MODULE_GATE:
			return tl_reply_gate_status(pControler, pPacket->arrData[0]);
		case TL_MODULE_LAMP:
			pLamp = (tl_controler_lamp_t*)pControler;
			if(pSim->iSimSegments >= 0)
			{
				iLampSegments = pLamp->stCurrentState.iState;
				pLamp->stCurrentState.iState = pSim->iSimSegments;
			}
			iRet = tl_wdresponce_lamp_command_set(pDbGlobal->stPort.arrSend, 
								pPacket->arrData[0], pControler->iId, pLamp );
			if(iLampSegments >= 0)	
				pLamp->stCurrentState.iState = iLampSegments;
			if(OK == iRet)
			{	
				pDbGlobal->stPort.iSendSize = strlen(pDbGlobal->stPort.arrSend);			
				return tl_com_send(&(pDbGlobal->stPort)); 
			}
			break;
		case TL_MODULE_TIMER:
			if(OK == tl_wdresponce_timer_command_set(pDbGlobal->stPort.arrSend, 
				pPacket->arrData[0], pControler->iId, (tl_controler_timer_t*)pControler))
			{	
					pDbGlobal->stPort.iSendSize = strlen(pDbGlobal->stPort.arrSend);
					return tl_com_send(&(pDbGlobal->stPort));
			}
			break;
		default:
			return ERROR;	
	}	
	return ERROR;
}

int tl_sim_pakcet_process(tl_com_t *pCom)
{
 int iRet;
 t_can_packet_t stPacket;
 tl_global_config_t *pGlobal = tl_get_global_dbase();	

 	if(pCom->iRcvSize < 1)
 		return ERROR;	 		
	if(TL_PKT_END == pCom->arrRecived[pCom->iRcvSize-1])
	{	
		if(TL_IS_DEBUG(RX))
		{
			tl_log_info("Recived packet from %s, line state is %d", 
				pCom->szPortName, pCom->iLineState );
			if(TL_IS_DEBUG(PKT_DUMP))
				tl_log_info("[%s]", pCom->arrRecived);
		}
		if(OK == tl_command_extract(pCom->arrRecived, pCom->iRcvSize, &stPacket))
		{
			if(	TL_ADDRESS_BCAST == stPacket.iAddress)
			{
				sim_tl_controler(TL_ADDRESS_BCAST, &stPacket);
			}else if(NULL != pGlobal->arrDevices[stPacket.iAddress] )
			{
				if(TL_IS_DEBUG(RX))
					tl_log_info("Recived packet %d: [%s]", stPacket.iCommand, pCom->arrRecived );
	 			if(	NULL != pGlobal->arrDevices[stPacket.iAddress]->fPktProcess)
					pGlobal->arrDevices[stPacket.iAddress]->fPktProcess(stPacket.iAddress, &stPacket);
			}
		}
	}
	else
		iRet = ERROR;

	pCom->iLineState = TL_COM_IDLE;
	return iRet;
}

int32_t sim_tl_com_read(thread_t *pThread)
{
	int iRet;
	tl_com_t *pCom = (tl_com_t*) pThread->pArg;
	
	pCom->pReadThread  = NULL;
	iRet = tl_com_read_packet(pCom);
	if(OK == iRet)
	{
		memcpy(pCom->arrSend, pCom->arrRecived, TL_MAX_PACKET_SERIAL_SIZE);
		pCom->iSendSize = pCom->iRcvSize;
		tl_com_send(pCom);
	
		tl_sim_pakcet_process(pCom);
	}
	else if(ERROR == iRet)
	{
		tl_log_info("Read error, exit the simulation");
		exit(1);
	}

	pCom->iRcvSize = 0;
	pCom->arrRecived[0]=0;

	pCom->pReadThread  = thread_read_add(pThread->pMain, sim_tl_com_read, pCom, pCom->iNetFd);
	return 1;
}

