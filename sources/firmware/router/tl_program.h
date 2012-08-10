#ifndef __TLIGHT_PROGRAM_H__
#define __TLIGHT_PROGRAM_H__

#include "tl_program_api.h"

#define TL_CMD_DELAY			"Delay"
#define TL_CMD_LOOP				"Loop"
#define TL_CMD_TIMER_CDOWN		"countdown"
#define TL_CMD_TIMER_DISPLAY	"display"
#define TL_CMD_TIMER_RED		"red"
#define TL_CMD_TIMER_GREEN		"green"

typedef struct
{
	char *szProgramFile;
	tl_instruction_t *arrInstructions;
	tl_program_state_t iState;
	tl_program_load_t iLoadMode;
	uint32_t iCount;
	uint32_t iCurrentStep;
	uint32_t iLoopStep;
	uint32_t iLoopCount;
	thread_t *pDelayThread;
}tl_program_t;

int tl_program_step(tl_program_t *pProgram);
int tl_program_stop(tl_program_t *pProgram);
int tl_program_reset(tl_program_t *pProgram);
int tl_program_change(tl_api_program_t *pNewProgram);

int tl_parse_prog_file(char *szFileName, tl_program_t **ppProgram);

#endif /* __TLIGHT_PROGRAM_H__ */


