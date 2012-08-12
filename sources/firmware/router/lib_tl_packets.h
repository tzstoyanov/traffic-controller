#ifndef __LIB_TLIGHT_PACKETS_H__
#define __LIB_TLIGHT_PACKETS_H__


int tl_timer_command_set(tl_instruction_t *pCommand);
int tl_lamp_command_set(tl_instruction_t *pCommand);
int tl_cfg_command_set(char *arrCommand, uint8_t iId, uint8_t iCfgId, uint8_t *arrData, uint8_t iDataSize);
int tl_wdrequest_command_set(char *arrCommand, uint8_t iDstId, uint8_t iSrcId);
int tl_failsafe_command_set(char *arrCommand);

int tl_wdresponce_lamp_command_set(char *arrCommand, 
			uint8_t iDstId, uint8_t iSrcId, tl_controler_lamp_t *pLamp);
int tl_wdresponce_timer_command_set(char *arrCommand, 
			uint8_t iDstId, uint8_t iSrcId, tl_controler_timer_t *pTimer);
int tl_wdresponce_gate_command_set(char *arrCommand, uint8_t iCommand,  
			uint8_t iDstId, uint8_t *arrSrcId, uint8_t *arrStatus, uint8_t iCount);
int tl_command_extract(char *arrCommand, uint32_t iLen, t_can_packet_t *pPacket);

#endif /* __LIB_TLIGHT_PACKETS_H__ */

