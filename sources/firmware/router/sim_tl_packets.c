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

int tl_reply_gate_status(tl_controler_t *pControler, uint8_t iDest)
{
	uint8_t iCount = 0, i, j=0, iCommand;
	uint8_t arrSlaves[TL_MAX_PACKET_CAN_DATA/2], arrStatus[TL_MAX_PACKET_CAN_DATA/2];
	tl_global_config_t *pDbGlobal = tl_get_global_dbase();			

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
				if(TL_MODULE_ERR_OK != pDbGlobal->arrDevices[i]->iStatus)
					arrStatus[j] = 	((pDbGlobal->arrDevices[i]->iType & 0x07) << 5) |
									((TL_MODULE_STATE_FAIL & 0x01) << 4) |
									(pDbGlobal->arrDevices[i]->iStatus & 0x0F);
				else if(TL_MODULE_LAMP != pDbGlobal->arrDevices[i]->iType)
					arrStatus[j] = 	((pDbGlobal->arrDevices[i]->iType & 0x07) << 5) |
									((TL_MODULE_STATE_OK & 0x01) << 4) |
									(pDbGlobal->arrDevices[i]->iStatus & 0x0F);
				 else
					arrStatus[j] = 	((pDbGlobal->arrDevices[i]->iType & 0x07) << 5) |
									((TL_MODULE_STATE_OK & 0x01) << 4) |
					(((tl_controler_lamp_t*)(pDbGlobal->arrDevices[i]))->stCurrentState.iState & 0x0F);
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

	if(NULL == pControler)
		return ERROR;

	if(1 != pPacket->iLen)
		return ERROR;	
	switch(pControler->iType)
	{
		case TL_MODULE_GATE:
			return tl_reply_gate_status(pControler, pPacket->arrData[0]);
		case TL_MODULE_LAMP:
			if(OK == tl_wdresponce_lamp_command_set(pDbGlobal->stPort.arrSend, 
								pPacket->arrData[0], pControler->iId, 
								(tl_controler_lamp_t*)pControler))
				{	
					pDbGlobal->stPort.iSendSize = strlen(pDbGlobal->stPort.arrSend);			
					return tl_com_send(&(pDbGlobal->stPort)); 
				}
			break;
		case TL_MODULE_TIMER:
			if(OK == tl_wdresponce_timer_command_set(pDbGlobal->stPort.arrSend, 
								pPacket->arrData[0], pControler->iId, 
								(tl_controler_timer_t*)pControler))
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
				pCom->szPortName, pCom->iLineSate );
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

	pCom->iLineSate = TL_COM_IDLE;
	return iRet;
}

int32_t sim_tl_com_read(thread_t *pThread)
{
	tl_com_t *pCom = (tl_com_t*) pThread->pArg;
	
	pCom->pReadThread  = NULL;
	
	tl_com_read_packet(pCom);
	
	memcpy(pCom->arrSend, pCom->arrRecived, TL_MAX_PACKET_SERIAL_SIZE);
	pCom->iSendSize = pCom->iRcvSize;
	tl_com_send(pCom);
	
	tl_sim_pakcet_process(pCom);

	pCom->iRcvSize = 0;
	pCom->arrRecived[0]=0;

	pCom->pReadThread  = thread_read_add(pThread->pMain, sim_tl_com_read, pCom, pCom->iNetFd);
	return 1;
}

