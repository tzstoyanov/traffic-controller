#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_packets_api.h"
#include "tl_api.h"
#include "tl_serial.h"
#include "tl_program.h"
#include "tl_controlers.h"
#include "lib_tl_log.h"
#include "lib_tl_config.h"
#include "lib_tl_packets.h"
#include "tl_main.h"

static uint32_t tl_get_file_lines(char *szFileName)
{
	uint32_t iCount = 0;
	char szLine[250];
	FILE *pFile;

	pFile = fopen((char*)szFileName, "r");
	if( NULL == pFile ) 
	    return 0;
	while( fgets((char*)szLine, 250, pFile))
		if('#' != szLine[0] && isalnum(szLine[0]) )
			iCount++;
	fclose(pFile);
	return iCount;	
}

int tl_parse_prog_lamp(tl_controler_lamp_t *pLamp, char **pszString, tl_instruction_t *pInstr)
{
	uint8_t iSegments=0, i=0;
	char *szString = NULL;

	if(TL_MODULE_LAMP != pLamp->stHead.iType)
		return ERROR;
	if(OK != tl_get_next_token(pszString, &szString) || NULL == szString)
		return ERROR;
	iSegments = strlen(szString);
	if(iSegments > TL_MAX_LAMP_SEGMENTS)
		return ERROR;
	pInstr->u.stCommand.state.stLamp.iState = 0;
	while(iSegments)
	{
		if('0' != szString[iSegments-1])
			pInstr->u.stCommand.state.stLamp.iState |= (1 << i);
		i++;
		iSegments--;
	}

	pInstr->iType = TL_INSTR_CMD;
	pInstr->u.stCommand.iDeviceId = pLamp->stHead.iId;
	pInstr->u.stCommand.iType = TL_MODULE_LAMP;
	if( OK !=  tl_lamp_command_set(pInstr))
		return ERROR;

	if(TL_IS_DEBUG(CONFIG))
		tl_log_info("Got instruction for %s, state 0x%X", 
			pLamp->stHead.szName, pInstr->u.stCommand.state.stLamp.iState);	

	return OK;
}

int tl_parse_prog_timer(tl_controler_timer_t *pTimer, char **pszString, tl_instruction_t *pInstr)
{
	int iDigit, j, iDataLen;
	char szHex[3];	
	char *szString = NULL;

	if(TL_MODULE_TIMER != pTimer->stHead.iType)
		return ERROR;
	if(OK != tl_get_next_token(pszString, &szString) || NULL == szString)
		return ERROR;
	if(0 == strncmp(szString, TL_CMD_TIMER_CDOWN, strlen(TL_CMD_TIMER_CDOWN)))
		pInstr->u.stCommand.state.stTimer.iAction = TL_TIMER_ACT_COUNTDOWN;
	else if(0 == strncmp(szString, TL_CMD_TIMER_DISPLAY, strlen(TL_CMD_TIMER_DISPLAY)))
		pInstr->u.stCommand.state.stTimer.iAction = TL_TIMER_ACT_STATIC;
	else
		return ERROR;
	if(OK != tl_get_next_token(pszString, &szString) || NULL == szString)
		return ERROR;
	iDataLen = 0;
	if(TL_TIMER_ACT_COUNTDOWN == pInstr->u.stCommand.state.stTimer.iAction)
	{			
		iDigit = atoi(szString);
		if(iDigit < 1 || iDigit > 255)
			return ERROR;
		pInstr->u.stCommand.state.stTimer.arrState[iDataLen++] = iDigit;	
	}	
	else
	{
		j=0;
		szHex[2] = 0;
		while(szString[j] && szString[j+1])
		{	
			szHex[0] = szString[j];
			szHex[1] = szString[j+1];
			iDigit = strtoul(szHex, NULL, 16);
			pInstr->u.stCommand.state.stTimer.arrState[iDataLen++] = iDigit;
			j+=2;
		}
	}
	pInstr->u.stCommand.state.stTimer.iStateLen = iDataLen;
	if(OK != tl_get_next_token(pszString, &szString) || NULL == szString)
		return ERROR;
	if(0 == strncmp(szString, TL_CMD_TIMER_RED, strlen(TL_CMD_TIMER_RED)))
		pInstr->u.stCommand.state.stTimer.iColor = TL_TIMER_COLOR_RED;
	else if(0 == strncmp(szString, TL_CMD_TIMER_GREEN, strlen(TL_CMD_TIMER_GREEN)))
		pInstr->u.stCommand.state.stTimer.iColor = TL_TIMER_COLOR_GREEN;
	else
		return ERROR;
	pInstr->iType = TL_INSTR_CMD;
	pInstr->u.stCommand.iDeviceId = pTimer->stHead.iId;
	pInstr->u.stCommand.iType = TL_MODULE_TIMER;
	if( OK !=  tl_timer_command_set(pInstr))
		return ERROR;

	if(TL_IS_DEBUG(CONFIG))
		tl_log_info("Got Timer %d, action 0x%X, color %d, state [0x%X 0x%X 0x%X], len %d", 
			pTimer->stHead.iId, pInstr->u.stCommand.state.stTimer.iAction, 
			pInstr->u.stCommand.state.stTimer.iColor, 
			pInstr->u.stCommand.state.stTimer.arrState[0],
			pInstr->u.stCommand.state.stTimer.arrState[1],
			pInstr->u.stCommand.state.stTimer.arrState[2],
			pInstr->u.stCommand.state.stTimer.iStateLen);	
			
	return OK;
}

int tl_parse_prog_file(char *szFileName, tl_program_t **ppProgram)
{
	uint32_t iCount = 0, iRet;
	uint32_t iInstrCount = tl_get_file_lines(szFileName);
	char szLine[250];
	char *szToken;
	FILE *pFile;
	char *szString;
	tl_controler_t *pControler;
	uint8_t iFail = 0;

	if(0 == iInstrCount || NULL == ppProgram)
		return ERROR;

	if(TL_IS_DEBUG(CONFIG))
		tl_log_info("Opening program file [%s] with %d instructions", szFileName, iInstrCount);	
		
	pFile = fopen((char*)szFileName, "r");
	if( NULL == pFile ) 
	    return ERROR;
	(*ppProgram) = calloc(1, sizeof(tl_program_t));
	if(NULL == *ppProgram)
	{
		fclose(pFile);
		return ERROR;
	}	
	(*ppProgram)->arrInstructions = calloc(iInstrCount, sizeof(tl_instruction_t));
	if(NULL == (*ppProgram)->arrInstructions)
	{	
		free(*ppProgram);
		*ppProgram = NULL;
		fclose(pFile);
		return ERROR;
	}
	
	(*ppProgram)->szProgramFile = strdup(szFileName);
	
	while( fgets((char*)szLine, 250, pFile))
	{
		if('#' == szLine[0] || !isalnum(szLine[0]) )
			continue;
		if(iCount >= iInstrCount)
		{
			iFail++;
			break;
		}
		if(TL_IS_DEBUG(CONFIG))
			tl_log_info("Analyzing line [%s], instruction %d", szLine, iCount);
		szToken = szLine;
		szString = NULL;
		if(OK != tl_get_next_token(&szToken, &szString) || NULL == szString)
		{
			iFail++;
			break;
		}
		if(0 == strncmp(szString, TL_CMD_DELAY, strlen(TL_CMD_DELAY)))
		{
			(*ppProgram)->arrInstructions[iCount].iType = TL_INSTR_DELAY;
			if(OK != tl_get_next_token(&szToken, &szString) || NULL == szString)
			{
				iFail++;			
				break;
			}
			(*ppProgram)->arrInstructions[iCount].u.iDelay = atoi(szString);
			if(TL_IS_DEBUG(CONFIG))
				tl_log_info("Got program delay [%d]", (*ppProgram)->arrInstructions[iCount].u.iDelay);			
		}else if(0 == strncmp(szString, TL_CMD_LOOP, strlen(TL_CMD_LOOP)))
		{
			(*ppProgram)->iLoopStep = iCount;
			if(TL_IS_DEBUG(CONFIG))
				tl_log_info("Got program loop at [%d]", (*ppProgram)->iLoopStep);	
			continue;
		}else if(0 == strncmp(szString, TL_CMD_EXIT, strlen(TL_CMD_EXIT)))
		{
			(*ppProgram)->iExitStep = iCount;
			if(TL_IS_DEBUG(CONFIG))
				tl_log_info("Got program exit at [%d]", (*ppProgram)->iExitStep);	
			continue;			
		}
		else 
		{
			pControler = tl_get_controler_byname(szString);
			if(NULL != pControler )
			{
				if(TL_IS_DEBUG(CONFIG))	
					tl_log_info("Found controler [%s] in local database", szString);			
				switch(pControler->iType)
				{
					case TL_MODULE_TIMER:
							iRet = tl_parse_prog_timer((tl_controler_timer_t*)pControler, &szToken, &((*ppProgram)->arrInstructions[iCount]));
						break;
					case TL_MODULE_LAMP:
							iRet = tl_parse_prog_lamp((tl_controler_lamp_t*)pControler, &szToken, &((*ppProgram)->arrInstructions[iCount]));
						break;
					default:
							iRet = ERROR;
						break;
				}
			}
			else
			{
				tl_log_error("Cannot find controler [%s] in local database", szString);
				iRet = ERROR;
			}
			if(OK != iRet)
			{
				iFail++;				
				break;	
			}			

		}
		iCount++;
	}
	(*ppProgram)->iCount = iCount;
	(*ppProgram)->iState = TL_PROGRAM_PAUSE;
	if((*ppProgram)->iExitStep)
		(*ppProgram)->iLoopEnd = (*ppProgram)->iExitStep;
	else	
		(*ppProgram)->iLoopEnd = (*ppProgram)->iCount;
	(*ppProgram)->iCurrentStep = 0;
	(*ppProgram)->iLoopCount = 0;
	fclose(pFile);
	if(0 == iFail)
	{
		if(TL_IS_DEBUG(CONFIG))	
			tl_log_info("Program in file [%s] loaded, ready for execution", szFileName);				
		return OK;
	}
	tl_log_error("Failed to parse program file [%s], prgram not loaded", szFileName);	
	free((*ppProgram)->arrInstructions);
	free((*ppProgram)->szProgramFile);
	free(*ppProgram);
	*ppProgram = NULL;
	return ERROR;
}
