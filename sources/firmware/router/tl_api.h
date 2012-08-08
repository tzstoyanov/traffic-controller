#ifndef __TLIGHT_API_H__
#define __TLIGHT_API_H__

#include "tl_log.h"
#include "tl_program_api.h"

#define TL_API_MAX_CONNECTIONS	5
#define TL_API_SOCKET_NAME	"/tmp/tlight_api_sock"

#define TL_API_SET_DEBUG_FLAG	1	/* uint32_t - debug flags */
#define TL_API_SET_DEBUG_DEST	2	/* tl_log_dest_t log destination */
#define TL_API_SET_PROGRAM		3	/* uint8_t mode, char* szName - file name with the new program, with full path to it */
#define TL_API_GET_STATUS		4 	/* tl_api_status_t */
#define TL_API_GET_STATUS_MOD	5	/* uint8_t num, num*tl_api_status_mod_type */
#define TL_API_GET_STATUS_PROG	6	/* uint8_t num, num*(tl_api_status_prog_t + iInstrCount*tl_api_status_instr_t) */

int tl_api_init();

typedef struct tl_api_status_type tl_api_status_t;
struct tl_api_status_type
{
	char szCrossName[128];
	char szComPort[64];
	int32_t iWdInterval;
	int32_t	iCommTimeout;
	uint32_t iDebugFlags;
	tl_log_dest_t iLogDest;
	char	szProgInProgress[128];
	uint8_t iDeviceNumber;
	uint8_t iDeviceOk;
	uint8_t iDeviceError;
}__attribute__((__packed__));

typedef struct tl_api_status_mod_type tl_api_status_mod_t;
struct tl_api_status_mod_type
{
	uint8_t	iAddress;
	uint8_t iType;
	char szName[64];
	uint8_t iGateId;
	uint8_t	iStatus;
	union
	{
		tl_lamp_state_t stLamp;
		tl_timer_state_t stTimer;
	}u;
}__attribute__((__packed__));

typedef struct tl_api_status_prog_type tl_api_status_prog_t;
struct tl_api_status_prog_type
{
	uint8_t iMode;
	uint8_t iState;
	uint32_t iRunCount;
	char	szFileName[512];
	uint32_t iInstrCount;
}__attribute__((__packed__));

typedef struct tl_api_status_instr_type tl_api_status_instr_t;
struct tl_api_status_instr_type
{
	tl_instruction_t stInstruction;
}__attribute__((__packed__));


#endif /* __TLIGHT_API_H__ */


