#ifndef __LIB_TLIGHT_PACKETS_H__
#define __LIB_TLIGHT_PACKETS_H__


int tl_timer_command_set(tl_instruction_t *pCommand);
int tl_lamp_command_set(tl_instruction_t *pCommand);
int tl_cfg_command_set(char *arrCommand, uint8_t iId, uint8_t iCfgId, uint8_t *arrData, uint8_t iDataSize);
int tl_wdrequest_command_set(char *arrCommand, uint8_t iDstId, uint8_t iSrcId);
int tl_failsafe_command_set(char *arrCommand);

int tl_com_pkt_process(tl_com_t *pCom);

#endif /* __LIB_TLIGHT_PACKETS_H__ */

