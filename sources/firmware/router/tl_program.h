#ifndef __TLIGHT_PROGRAM_H__
#define __TLIGHT_PROGRAM_H__

typedef struct
{
	uint8_t	iState;
}tl_lamp_state_t;

typedef struct
{
	uint8_t	iCurrentAction;
	uint8_t	iCurrentColor;	
	uint8_t	arrCurrentState[TL_MAX_TIMER_SEGMENTS];
	uint8_t	iStateLen;
}tl_timer_state_t;

typedef enum
{
	TL_INSTR_CMD,
	TL_INSTR_DELAY
}tl_instruction_type_t;

typedef struct
{
	char arrCommand[TL_MAX_PACKET_SERIAL_SIZE ];
	uint8_t iDeviceId;
	union
	{
		tl_lamp_state_t stLamp;
		tl_timer_state_t stTimer;
	}state;
}tl_instruction_cmd_t;

typedef struct
{
	tl_instruction_type_t iType;
	union
	{	
		uint32_t iDelay;
		tl_instruction_cmd_t stCommand;
	}u;
}tl_instruction_t;

typedef struct
{
	tl_instruction_t *arrInstructions;
	uint32_t iCount;
	uint32_t iCurrentStep;
	uint32_t iLoopStep;
	thread_t *pDelayThread;
}tl_program_t;

int tl_program_step(tl_program_t *pProgram);

#endif /* __TLIGHT_PROGRAM_H__ */


