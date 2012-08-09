#ifndef __TLIGHT_PROGRAM_H__
#define __TLIGHT_PROGRAM_H__

#include "tl_program_api.h"

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

#endif /* __TLIGHT_PROGRAM_H__ */


