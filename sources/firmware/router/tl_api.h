#ifndef __TLIGHT_API_H__
#define __TLIGHT_API_H__

#include "tl_packets_api.h"
#include "tl_program_api.h"
#include "lib_tl_log.h"

#define TL_API_MAX_CONNECTIONS	5
#define TL_API_SOCKET_NAME	"/tmp/tlight_api_sock"

#define TL_API_SET_DEBUG_FLAG	1	/* uint32_t - debug flags */
#define TL_API_SET_DEBUG_DEST	2	/* tl_log_dest_t log destination */
#define TL_API_SET_PROGRAM		3	/* tl_api_program_t - load type and file name of the new program, with full path to it */
#define TL_API_GET_STATUS		4 	/* tl_api_status_t */
#define TL_API_GET_STATUS_MOD	5	/* uint8_t num, num*tl_api_status_mod_type */
#define TL_API_GET_STATUS_PROG	6	/* uint8_t num, num*(tl_api_status_prog_t + iInstrCount*tl_api_status_instr_t) */
#define TL_API_STOP				8	/* Stop application */

int tl_api_init();

typedef struct tl_api_program_type tl_api_program_t;
struct tl_api_program_type
{
	tl_program_load_t iType;
	char	szProgName[128];
}__attribute__((__packed__));

typedef struct tl_api_status_type tl_api_status_t;
struct tl_api_status_type
{
	char szCrossName[128];
	char szComPort[64];
	int32_t iWdInterval;
	int32_t iDeadCount;
	int32_t	iCommTimeout;
	uint32_t iDebugFlags;
	tl_log_dest_t iLogDest;
	char	szProgLoaded[128];
	tl_program_state_t iProgState;
	uint32_t iProgLoop;
	uint8_t iDeviceNumber;
	uint8_t iDeviceOk;
	uint8_t iDeviceError;
	uint32_t iCritError;
	uint32_t iWarning;
	uint32_t iLineErrors;
	struct timeval stUpTime;
}__attribute__((__packed__));

typedef struct tl_api_status_mod_type tl_api_status_mod_t;
struct tl_api_status_mod_type
{
	uint8_t	iAddress;
	uint8_t iType;
	char szName[64];
	uint8_t iGateId;
	uint8_t	iStatus;
	uint8_t	iCritical;
	uint32_t iErrorCount;
	uint8_t iWdMiss;
	uint8_t bLastCommandValid;
	char arrLastCommand[TL_MAX_PACKET_SERIAL_SIZE];	
	union
	{
		tl_lamp_state_t stLamp;
		tl_timer_state_t stTimer;
	}u;
}__attribute__((__packed__));

typedef struct tl_api_status_prog_type tl_api_status_prog_t;
struct tl_api_status_prog_type
{
	tl_program_load_t iLoadMode;
	tl_program_state_t iState;
	uint32_t iLoopCount;
	uint32_t iLoopStep;
	uint32_t iExitStep;
	char	szFileName[512];
	uint32_t iInstrCount;
}__attribute__((__packed__));

typedef struct tl_api_status_instr_type tl_api_status_instr_t;
struct tl_api_status_instr_type
{
	tl_instruction_t stInstruction;
}__attribute__((__packed__));


#endif /* __TLIGHT_API_H__ */


