#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_packets_api.h"
#include "tl_serial_api.h"
#include "tl_program_api.h"
#include "tl_controlers_api.h"
#include "lib_tl_config.h"
#include "lib_tl_packets.h"
#include "lib_tl_log.h"

uint8_t tl_char2hex(uint8_t iChar)
{
	if(iChar <= '9' && iChar >= '0')
		return iChar - '0';
	if(iChar <= 'F' && iChar >= 'A')
		return 10 + (iChar - 'A');
	return 0;
}

uint8_t tl_hex2char(uint8_t iChar)
{
	if(iChar < 10)
		return ('0' + iChar);
	if(iChar < 16)
		return ('A' + (iChar - 10));
	return 0;
}

int tl_can_pkt_checksum(char *arrCmd, char *arrCsum, uint8_t iSize)
{
	uint8_t iSum=0, i;
	for(i=0; i<iSize; i++)
		iSum += arrCmd[i];
	iSum = ~iSum+1;
	arrCsum[0] = tl_hex2char((iSum & 0xF0) >> 4);
	arrCsum[1] = tl_hex2char(iSum & 0x0F);	
	return OK;
}

int tl_can_pkt_checksum_check(char *arrCmd, uint8_t iSize)
{
	char arrCsum[2];

	if(OK != tl_can_pkt_checksum(arrCmd, arrCsum, iSize))
		return ERROR;
	if(arrCmd[iSize] != arrCsum[0])
		return ERROR;
	if(arrCmd[iSize+1] != arrCsum[1])
		return ERROR;
	return OK;
}

int tl_can_pkt_checksum_set(char *arrCmd, uint8_t iSize)
{
	char arrCsum[2];

	if(OK != tl_can_pkt_checksum(arrCmd, arrCsum, iSize))
		return ERROR;

	arrCmd[iSize] = arrCsum[0];
	arrCmd[iSize+1] = arrCsum[1];
	return OK;
}

int	tl_can_pkt_encode(t_can_packet_t *pCan, char *arrCmd)
{
	uint8_t i=0,j;

	arrCmd[i++] = TL_PKT_START;
	arrCmd[i++] = tl_hex2char(pCan->iCommand & 0x0F);
	arrCmd[i++] = tl_hex2char((pCan->iAddress & 0xF0) >> 4);
	arrCmd[i++] = tl_hex2char(pCan->iAddress & 0x0F);
	arrCmd[i++] = tl_hex2char(pCan->iLen & 0x0F);
	for(j=0; j<pCan->iLen; j++)
	{
		arrCmd[i++] = tl_hex2char((pCan->arrData[j] & 0xF0) >> 4);
		arrCmd[i++] = tl_hex2char(pCan->arrData[j] & 0x0F);		
	}
	tl_can_pkt_checksum_set(arrCmd+1, i-1);
	i+=2;
	arrCmd[i++] = TL_PKT_END;	
	arrCmd[i++] = 0;
	return OK;
}

int tl_command_extract(char *arrCommand, uint32_t iLen, t_can_packet_t *pPacket)
{
	uint8_t i, j;

	i = 0;	 
	if(iLen < 8 || TL_PKT_START != arrCommand[i++] || TL_PKT_END != arrCommand[iLen-1])
	{
		if(TL_IS_DEBUG(RX))
			tl_log_error("Received packet with broken header / lenght, discarding");
		return ERROR;
	}
	if(OK != tl_can_pkt_checksum_check(arrCommand+1, iLen - 4))
	{
		if(TL_IS_DEBUG(RX))
			tl_log_error("[%s]Received packet with broken checksum, discarding", arrCommand);
		return ERROR;			
	}
	pPacket->iCommand = tl_char2hex(arrCommand[i++]);
	pPacket->iAddress = ((tl_char2hex(arrCommand[i++]) & 0x0F) << 4 );
	pPacket->iAddress |= (tl_char2hex(arrCommand[i++]) & 0x0F);
	pPacket->iLen = tl_char2hex(arrCommand[i++]);
	for(j=0; j<pPacket->iLen; j++ )
	{
		if(i >= (iLen - 3))
		{
			if(TL_IS_DEBUG(RX))
				tl_log_error("Received packet with broken format, discarding");		
			return ERROR;
		}
		pPacket->arrData[j] = ((tl_char2hex(arrCommand[i++]) & 0x0F) << 4 );
		pPacket->arrData[j] |= (tl_char2hex(arrCommand[i++]) & 0x0F);
	}
	return OK;
}

int tl_timer_command_set(tl_instruction_t *pCommand)
{
	uint8_t i;
	t_can_packet_t stPacket;

	bzero(&stPacket, sizeof(stPacket));
	stPacket.iCommand = TL_CMD_STATE;
	stPacket.iAddress = pCommand->u.stCommand.iDeviceId;
	stPacket.iLen = 1;
	stPacket.arrData[0] = 
				((TL_MODULE_TIMER & 0x07) << 5) | 
				(( pCommand->u.stCommand.state.stTimer.iColor & 0x03) << 3) | 
				( pCommand->u.stCommand.state.stTimer.iAction & 0x03);
	if(	pCommand->u.stCommand.state.stTimer.iStateLen && 
		pCommand->u.stCommand.state.stTimer.iStateLen < TL_MAX_TIMER_SEGMENTS)
	{
		stPacket.iLen += pCommand->u.stCommand.state.stTimer.iStateLen;
		for(i=0; i<pCommand->u.stCommand.state.stTimer.iStateLen; i++)
			stPacket.arrData[1+i] =  pCommand->u.stCommand.state.stTimer.arrState[i];	
	}
	return	tl_can_pkt_encode(&stPacket, pCommand->u.stCommand.arrCommand);
}

int tl_lamp_command_set(tl_instruction_t *pCommand)
{
	t_can_packet_t stPacket;
	bzero(&stPacket, sizeof(stPacket));
	
	stPacket.iCommand = TL_CMD_STATE;
	stPacket.iAddress = pCommand->u.stCommand.iDeviceId;
	stPacket.iLen = 1;	
	stPacket.arrData[0] = ((TL_MODULE_LAMP & 0x07) << 5) | 
						(pCommand->u.stCommand.state.stLamp.iState & 0x0F);
	
	return	tl_can_pkt_encode(&stPacket, pCommand->u.stCommand.arrCommand);
}

int tl_failsafe_command_set(char *arrCommand)
{
	t_can_packet_t stPacket;
	bzero(&stPacket, sizeof(stPacket));
	
	stPacket.iCommand = TL_CMD_STATE;
	stPacket.iAddress = TL_ADDRESS_BCAST;
	stPacket.iLen = 1;
	stPacket.arrData[0] = (	(TL_MODULE_ALL & 0x07) << 5) | 
							(TL_ALL_STATE_FAILSAFE & 0x0F);
	
	return	tl_can_pkt_encode(&stPacket, arrCommand);
}

int tl_wdrequest_command_set(char *arrCommand, uint8_t iDstId, uint8_t iSrcId)
{
	t_can_packet_t stPacket;
	bzero(&stPacket, sizeof(stPacket));
	
	stPacket.iCommand = TL_CMD_WD_REQUEST;
	stPacket.iAddress = iDstId;
	stPacket.iLen = 1;
	stPacket.arrData[0] = iSrcId;
	
	return	tl_can_pkt_encode(&stPacket, arrCommand);
}

int tl_wdresponce_lamp_command_set(char *arrCommand, 
			uint8_t iDstId, uint8_t iSrcId, tl_controler_lamp_t *pLamp)
{
	t_can_packet_t stPacket;
	bzero(&stPacket, sizeof(stPacket));
	
	stPacket.iCommand = TL_CMD_WD_RESPONCE;
	stPacket.iAddress = iDstId;
	stPacket.iLen = 2;
	stPacket.arrData[0] = iSrcId;
	if(TL_MODULE_ERR_OK == pLamp->stHead.iStatus)
		stPacket.arrData[1] = 	((TL_MODULE_LAMP & 0x07) << 5) |
								((TL_MODULE_STATE_OK & 0x01) << 4) |
								(pLamp->stCurrentState.iState & 0x0F);
	 else
		stPacket.arrData[1] =	((TL_MODULE_LAMP & 0x07) << 5) |
								((TL_MODULE_STATE_FAIL & 0x01) << 4) |
								(pLamp->stHead.iStatus & 0x0F);
	 							
	return	tl_can_pkt_encode(&stPacket, arrCommand);
}

int tl_wdresponce_timer_command_set(char *arrCommand, 
			uint8_t iDstId, uint8_t iSrcId, tl_controler_timer_t *pTimer)
{
	uint8_t iStatus;
	t_can_packet_t stPacket;
	bzero(&stPacket, sizeof(stPacket));
	
	stPacket.iCommand = TL_CMD_WD_RESPONCE;
	stPacket.iAddress = iDstId;
	stPacket.iLen = 2;
	stPacket.arrData[0] = iSrcId;
	if(TL_MODULE_ERR_OK == pTimer->stHead.iStatus)
		iStatus = TL_MODULE_STATE_OK;
	else	
		iStatus = TL_MODULE_STATE_FAIL;
	stPacket.arrData[1] =  	((TL_MODULE_TIMER & 0x07) << 5) |
							((iStatus & 0x01) << 4) |
							(pTimer->stHead.iStatus & 0x0F);
	 								
	return	tl_can_pkt_encode(&stPacket, arrCommand);
}

int tl_wdresponce_gate_command_set(char *arrCommand, uint8_t iCommand,  
			uint8_t iDstId, uint8_t *arrSrcId, uint8_t *arrStatus, uint8_t iCount)
{
	uint8_t i,j=0;
	t_can_packet_t stPacket;
	bzero(&stPacket, sizeof(stPacket));
	
	stPacket.iCommand = iCommand;
	stPacket.iAddress = iDstId;
	stPacket.iLen = 0;
	for(i=0; i<iCount; i++)
	{
		if(stPacket.iLen > (TL_MAX_PACKET_CAN_DATA-2))
			return ERROR;
		stPacket.arrData[j++] = arrSrcId[i];
		stPacket.arrData[j++] = arrStatus[i];
		stPacket.iLen += 2;
	}	
	 								
	return	tl_can_pkt_encode(&stPacket, arrCommand);
}			


int tl_cfg_command_set(char *arrCommand, uint8_t iId, uint8_t iCfgId, uint8_t *arrData, uint8_t iDataSize)
{
	uint8_t i;
	t_can_packet_t stPacket;
	bzero(&stPacket, sizeof(stPacket));
	
	stPacket.iCommand = TL_CMD_CONFIG;
	stPacket.iAddress = iId;
	stPacket.iLen = 1;
	stPacket.arrData[0] = ((iCfgId & 0x0F) << 4) | ((iDataSize & 0x0F));
	if(iDataSize && arrData && iDataSize < 8)
	{
		stPacket.iLen += iDataSize;
		for(i=0; i<iDataSize; i++)
			stPacket.arrData[1+i] = arrData[i];	
	}
	
	return	tl_can_pkt_encode(&stPacket, arrCommand);
}

