#ifndef __TLIGHT_PROGRAM_H__
#define __TLIGHT_PROGRAM_H__

#include "tl_program_api.h"

typedef struct
{
	char *szProgramFile;
	tl_instruction_t *arrInstructions;
	uint32_t iCount;
	uint32_t iCurrentStep;
	uint32_t iLoopStep;
	thread_t *pDelayThread;
}tl_program_t;

int tl_program_step(tl_program_t *pProgram);

#endif /* __TLIGHT_PROGRAM_H__ */


