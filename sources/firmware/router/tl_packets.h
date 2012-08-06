#ifndef __TLIGHT_PACKETS_H__
#define __TLIGHT_PACKETS_H__

#define TL_MODULE_MASTER	0
#define TL_MODULE_GATE		1
#define TL_MODULE_LAMP		2
#define TL_MODULE_TIMER		3

#define TL_CMD_CONFIG			1
#define TL_CMD_STATE			2
#define TL_CMD_WD_REQUEST		3
#define TL_CMD_WD_RESPONCE		4
#define TL_CMD_WD_RESPONCE_F	5

#define TL_CFGID_QINTERVAL		1
#define TL_CFGID_SLAVES			2

#define TL_LAMP_STATE_OFF		0
#define TL_LAMP_STATE_ON		1

#define TL_TIMER_COLOR_GREEN	0
#define TL_TIMER_COLOR_RED		1

#define TL_TIMER_ACT_COUNTDOWN	0
#define TL_TIMER_ACT_STATIC		1

#define TL_MODULE_STATE_OK		0
#define TL_MODULE_STATE_FAIL	1

#define TL_MODULE_ERR_OK		0
#define TL_MODULE_ERR_INIT		1
#define TL_MODULE_ERR_HW		2
#define TL_MODULE_ERR_COMM		3
#define TL_MODULE_ERR_SLAVE		4
#define TL_MODULE_ERR_TIMEOUT	5

#define TL_PKT_START	':'
#define TL_PKT_END		'\n'

#define TL_ADDRESS_BCAST	0xFF

typedef struct
{
	uint8_t	iCommand;
	uint8_t	iAddress;
	uint8_t	iLen;
	uint8_t	arrData[8];	
}t_can_packet_t;

int tl_timer_command_set(tl_instruction_t *pCommand);
int tl_lamp_command_set(tl_instruction_t *pCommand);
int tl_cfg_command_set(char *arrCommand, uint8_t iId, uint8_t iCfgId, uint8_t *arrData, uint8_t iDataSize);
int tl_wdrequest_command_set(char *arrCommand, uint8_t iDstId, uint8_t iSrcId);
int tl_com_pkt_process(tl_com_t *pCom);

#endif /* __TLIGHT_PACKETS_H__ */
