#ifndef __TLIGHT_PACKETS_API_H__
#define __TLIGHT_PACKETS_API_H__

#define TL_MODULE_MASTER	0
#define TL_MODULE_GATE		1
#define TL_MODULE_LAMP		2
#define TL_MODULE_TIMER		3
#define TL_MODULE_ALL		4

#define TL_CMD_CONFIG			1
#define TL_CMD_STATE			2
#define TL_CMD_WD_REQUEST		3
#define TL_CMD_WD_RESPONCE		4
#define TL_CMD_WD_RESPONCE_F	5

#define TL_CFGID_QINTERVAL		1
#define TL_CFGID_DEADCOUNT		2
#define TL_CFGID_COMTIMEOUT		3
#define TL_CFGID_SLAVES			4

#define TL_ALL_STATE_FAILSAFE	1

#define TL_LAMP_STATE_OFF		0
#define TL_LAMP_STATE_ON		1

#define TL_TIMER_COLOR_GREEN	0
#define TL_TIMER_COLOR_RED		1

#define TL_TIMER_ACT_STATIC		0
#define TL_TIMER_ACT_COUNTDOWN	1

#define TL_MODULE_STATE_OK		0
#define TL_MODULE_STATE_FAIL	1

#define TL_MODULE_ERR_OK		0
#define TL_MODULE_ERR_INIT		1
#define TL_MODULE_ERR_HW_ERR	2
#define TL_MODULE_ERR_HW_WARN	3
#define TL_MODULE_ERR_COMM		4
#define TL_MODULE_ERR_SLAVE		5
#define TL_MODULE_ERR_TIMEOUT	6
#define TL_MODULE_ERR_MISMATCH	7

#define TL_PKT_START	':'
#define TL_PKT_END		';'

#define TL_ADDRESS_BCAST	0xFF

#define TL_MAX_PACKET_SERIAL_SIZE 	25
#define TL_MAX_PACKET_CAN_SIZE 		11
#define TL_MAX_PACKET_CAN_DATA 		8

typedef struct
{
	uint8_t	iCommand;
	uint8_t	iAddress;
	uint8_t	iLen;
	uint8_t	arrData[TL_MAX_PACKET_CAN_DATA];	
}t_can_packet_t;

#endif /* __TLIGHT_PACKETS_API_H__ */

