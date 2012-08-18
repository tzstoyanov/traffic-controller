#ifndef __TLIGHT_COMMON_API_H__
#define __TLIGHT_COMMON_API_H__

#define TL_MODULE_MASTER	0	/* Module Master */
#define TL_MODULE_GATE		1	/* Module Gateway */
#define TL_MODULE_LAMP		2	/* Module Lamp */
#define TL_MODULE_TIMER		3	/* Module Timer */
#define TL_MODULE_ALL		4	/* All Modules  */

#define TL_CMD_CONFIG			1	/* Config packet */
#define TL_CMD_STATE			2	/* State command */
#define TL_CMD_WD_REQUEST		3	/* WatchDog request packet */
#define TL_CMD_WD_RESPONCE		4	/* WatchDog responce packet */
#define TL_CMD_WD_RESPONCE_F	5	/* WatchDog responce fragment packet */

#define TL_CFGID_QINTERVAL		1	/* Config parameter WatchDog Interval */
#define TL_CFGID_DEADCOUNT		2	/* Config parameter Dead Count */
#define TL_CFGID_COMTIMEOUT		3	/* Config parameter Communication Timeout */
#define TL_CFGID_SLAVES			4	/* Config parameter Slaves List */

#define TL_ALL_STATE_FAILSAFE	1	/* FailSafe State */

#define TL_LAMP_STATE_OFF		0	/* Lamp Segment State OFF */
#define TL_LAMP_STATE_ON		1	/* Lamp Segment State ON */

#define TL_TIMER_COLOR_GREEN	0	/* Timer Segment Color Green */
#define TL_TIMER_COLOR_RED		1	/* Timer Segment Color Red */

#define TL_TIMER_ACT_STATIC		0	/* Timer Action Display Static */
#define TL_TIMER_ACT_COUNTDOWN	1	/* Timer Action Countdown */

#define TL_MODULE_STATUS_OK		0	/* Module Status OK */
#define TL_MODULE_STATUS_FAIL	1	/* Module Status Fail */

#define TL_MODULE_STATE_OK			0	/* Module State OK */
#define TL_MODULE_STATE_INIT		1	/* Module State Init */
#define TL_MODULE_STATE_HW_ERR		2	/* Module State Hardware Error */
#define TL_MODULE_STATE_HW_WARN		3	/* Module State Hardware Warning */
#define TL_MODULE_STATE_COMM_ERR	4	/* Module State Communication Error */
#define TL_MODULE_STATE_TIMEOUT		5	/* Module State Timeout */
#define TL_MODULE_STATE_MISMATCH	6	/* Module State Segments State Mismatch */

#define TL_PKT_START	':'		/* Start Packet symbol, used in serial port communicaion */
#define TL_PKT_END		';'		/* End Packet symbol, used in serial port communicaion */

#define TL_ADDRESS_BCAST	0xFF	/* Broadcast Address */

#define TL_MAX_PACKET_SERIAL_SIZE 	25	/* Max packet size on serial line */
#define TL_MAX_PACKET_CAN_SIZE 		11	/* Max CAN packet size */
#define TL_MAX_PACKET_CAN_DATA 		8	/* Max CAN payload size */

#endif /* __TLIGHT_COMMON_API_H__ */

