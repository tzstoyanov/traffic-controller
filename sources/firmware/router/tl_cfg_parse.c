#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_api.h"
#include "tl_serial.h"
#include "tl_program.h"
#include "tl_packets.h"
#include "tl_controlers.h"
#include "tl_log.h"
#include "main_tlight.h"
#include "tl_config.h"

static int tl_get_next_token(char **pszLine, char **pszString)
{
	int i=0, j=0;
	while(0 != (*pszLine)[i] && !isalnum((*pszLine)[i]) && '/' != (*pszLine)[i]) i++;
	if(0 == (*pszLine)[i])
		return ERROR;
	j=i;
	while(isalnum((*pszLine)[j]) || '.' == ((*pszLine)[j]) || '/' == ((*pszLine)[j])) j++;
	if(0 == (*pszLine)[j])
		return ERROR;
	(*pszString) = ((*pszLine) + i);
	(*pszLine)[j] = 0;
	(*pszLine) = ((*pszLine) + j + 1);
	return OK;
}

int tl_parse_port_cfg(char *szPortCfg, tl_global_t *pGlobal)
{
	int i=0, iLen = strlen(szPortCfg);

	pGlobal->stPort.iPortFlags = 0;
	
	while(szPortCfg[i] && '/' != szPortCfg[i]) i++;
	if('/' != szPortCfg[i] || (i+3) >= iLen)
		return ERROR;
	szPortCfg[i] = 0;
	
	if(0 == strncmp(szPortCfg, "9600", strlen("9600")))
		pGlobal->stPort.iPortSpeed	= B9600;
	else 	if(0 == strncmp(szPortCfg, "19200", strlen("19200")))
			pGlobal->stPort.iPortSpeed	= B19200;
	else 	if(0 == strncmp(szPortCfg, "38400", strlen("38400")))
			pGlobal->stPort.iPortSpeed	= B38400;
	else 	if(0 == strncmp(szPortCfg, "57600", strlen("57600")))
			pGlobal->stPort.iPortSpeed	= B57600;
	else 	if(0 == strncmp(szPortCfg, "115200", strlen("115200")))
			pGlobal->stPort.iPortSpeed	= B115200;
	else 	if(0 == strncmp(szPortCfg, "230400", strlen("230400")))
			pGlobal->stPort.iPortSpeed	= B230400;
	else 	if(0 == strncmp(szPortCfg, "460800", strlen("460800")))
			pGlobal->stPort.iPortSpeed	= B460800;
	else 	if(0 == strncmp(szPortCfg, "500000", strlen("500000")))
			pGlobal->stPort.iPortSpeed	= B500000;
	else 	if(0 == strncmp(szPortCfg, "576000", strlen("576000")))
			pGlobal->stPort.iPortSpeed	= B576000;
	else 	if(0 == strncmp(szPortCfg, "921600", strlen("921600")))
			pGlobal->stPort.iPortSpeed	= B921600;
	else 	if(0 == strncmp(szPortCfg, "1000000", strlen("1000000")))
			pGlobal->stPort.iPortSpeed	= B1000000;
	else 	if(0 == strncmp(szPortCfg, "1152000", strlen("1152000")))
			pGlobal->stPort.iPortSpeed	= B1152000;
	else 	if(0 == strncmp(szPortCfg, "1500000", strlen("1500000")))
			pGlobal->stPort.iPortSpeed	= B1500000;
	else return ERROR;

	switch(szPortCfg[i+1])
	{
		case '5':
				pGlobal->stPort.iPortFlags |= CS5;
			break;
		case '6':
				pGlobal->stPort.iPortFlags |= CS6;
			break;
		case '7':
				pGlobal->stPort.iPortFlags |= CS7;
			break;
		case '8':
				pGlobal->stPort.iPortFlags |= CS8;
			break;
		default:
			return ERROR;
	}	

	switch(szPortCfg[i+2])
	{
		case 'N':
			break;
		case 'E':
				pGlobal->stPort.iPortFlags |= PARENB;
			break; 
		case 'O':
				pGlobal->stPort.iPortFlags |= PARENB;
				pGlobal->stPort.iPortFlags |= PARODD;
			break;
		default:
			return ERROR;
	}
	
	if(TL_IS_DEBUG(CONFIG))
		tl_log_info("Got port speed %s (0x%X), data %c , parity %c (0x%X)", 
					szPortCfg, szPortCfg[i+1], szPortCfg[i+2], pGlobal->stPort.iPortFlags) ;
	
	return OK;	
}

int tl_parse_cfg_file(char *szFileName, tl_global_t *pGlobal)
{
	char szLine[250];
	char *szToken;
	FILE *pFile;
	char *szString;
	int iId, iLen;	
	uint8_t iGateId = 0, iType;
	tl_controler_t *pControler;

	if(TL_IS_DEBUG(CONFIG))
		tl_log_info("Opening config file [%s]", szFileName);

	pFile = fopen((char*)szFileName, "r");
	if( NULL == pFile ) 
	    return ERROR;
	while( fgets((char*)szLine, 250, pFile))
	{
		if('#' == szLine[0] || !isalnum(szLine[0]) )
			continue;
		szToken = szLine;
		szString = NULL;
		if(TL_IS_DEBUG(CONFIG))
			tl_log_info("Analyzing line [%s]", szLine);
		
		if(OK != tl_get_next_token(&szToken, &szString) || NULL == szString)
			continue;
			
		if(0 == strncmp(szString, TL_PARAM_SYS_NAME, strlen(TL_PARAM_SYS_NAME)))
		{
			iLen = strlen(szToken);
			while(iLen && !isalnum(szToken[iLen]) && !isblank(szToken[iLen]))
			{
				szToken[iLen] = 0;
				iLen--;	
			}	
			pGlobal->szCrossName = strdup(szToken);
			if(TL_IS_DEBUG(CONFIG))
				tl_log_info("Got systme name [%s]", pGlobal->szCrossName);
		}else if(0 == strncmp(szString, TL_PARAM_COM, strlen(TL_PARAM_COM)))
		{
			if(OK != tl_get_next_token(&szToken, &szString) || NULL == szString)
				continue;
			pGlobal->stPort.szPortName = strdup(szString);
			if(TL_IS_DEBUG(CONFIG))
				tl_log_info("Got port name [%s]", pGlobal->stPort.szPortName);			
		}else if(0 == strncmp(szString, TL_PARAM_COM_CFG, strlen(TL_PARAM_COM_CFG)))
		{
			szString = NULL;
			if(OK != tl_get_next_token(&szToken, &szString) || NULL == szString)
				continue;
			if(TL_IS_DEBUG(CONFIG))
				tl_log_info("Got port params [%s], going to parse it", szString);							
			if(OK != tl_parse_port_cfg(szString, pGlobal))
				continue;
		}else if(0 == strncmp(szString, TL_PARAM_WD, strlen(TL_PARAM_WD)))
		{
			szString = NULL;
			if(OK != tl_get_next_token(&szToken, &szString) || NULL == szString)
				continue;
			pGlobal->iWdInterval = atoi(szString);	
			if(TL_IS_DEBUG(CONFIG))
				tl_log_info("Got WD interval [%d]", pGlobal->iWdInterval);
		}else if(0 == strncmp(szString, TL_PARAM_TIMEOUT, strlen(TL_PARAM_TIMEOUT)))
		{
			szString = NULL;
			if(OK != tl_get_next_token(&szToken, &szString) || NULL == szString)
				continue;
			pGlobal->iTimeout = atoi(szString);
			if(TL_IS_DEBUG(CONFIG))
				tl_log_info("Got Timeout interval [%d]", pGlobal->iTimeout);			
		}else if(0 == strncmp(szString, TL_PARAM_MODULE, strlen(TL_PARAM_MODULE)))
		{
			szString = NULL;
			if(OK != tl_get_next_token(&szToken, &szString) || NULL == szString)
				continue;
			if(TL_IS_DEBUG(CONFIG))
				tl_log_info("Got Module [%s], going to parse it", szString);
			if(0 == strncmp(szString, TL_PARAM_MASTER, strlen(TL_PARAM_MASTER)))
				iType = TL_MODULE_MASTER;
			else if(0 == strncmp(szString, TL_PARAM_GATE, strlen(TL_PARAM_GATE)))
				iType = TL_MODULE_GATE;
			else if(0 == strncmp(szString, TL_PARAM_LAMP, strlen(TL_PARAM_LAMP)))
				iType = TL_MODULE_LAMP;
			else if(0 == strncmp(szString, TL_PARAM_TIMER, strlen(TL_PARAM_TIMER)))
				iType = TL_MODULE_TIMER;
			else
			{
				tl_log_error("Unkown module [%s]", szString);
				continue;
			}
			pControler = tl_NewControler(iType);
			if(NULL == pControler)
				continue;
			szString = NULL;
			if(OK != tl_get_next_token(&szToken, &szString) || NULL == szString)
			{	
				free(pControler);
				continue;
			}
			pControler->szName = strdup(szString);
			
			if(OK != tl_get_next_token(&szToken, &szString) || NULL == szString)
			{	
				free(pControler);
				continue;
			}
			iId = atoi(szString);
			if(iId < 1 || iId > TL_MAX_CONTROLERS)
			{
				tl_log_error("Invalid controler ID %s (%d)", szString, iId);
				free(pControler);				
				continue;
			}
			pControler->iId = iId;
			if(NULL != pGlobal->arrDevices[iId])
			{	
				tl_log_error("Controler with ID %d already configured", iId);
				free(pControler);
				continue;
			}
			pGlobal->arrDevices[iId] = pControler;
			if(TL_IS_DEBUG(CONFIG))
				tl_log_info("New Module configured: type %d, name %s, address %d", 
										iType, pControler->szName, iId);			
			if(TL_MODULE_MASTER == pControler->iType)
				continue;
			if(TL_MODULE_GATE == pControler->iType)
			{	
				iGateId = pControler->iId;
				continue;
			}
			pControler->iGateId = iGateId;
			if(NULL != pGlobal->arrDevices[iGateId])
				pGlobal->arrDevices[iGateId]->iCritical += pControler->iCritical;
			if(TL_IS_DEBUG(CONFIG))
				tl_log_info("The module is attached to gate %d", iGateId);	
		}else
			tl_log_error("Unkown parameter %s", szString);
	}
	fclose(pFile);
	return OK;
}

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
		pInstr->u.stCommand.state.stTimer.iCurrentAction = TL_TIMER_ACT_COUNTDOWN;
	else if(0 == strncmp(szString, TL_CMD_TIMER_DISPLAY, strlen(TL_CMD_TIMER_DISPLAY)))
		pInstr->u.stCommand.state.stTimer.iCurrentAction = TL_TIMER_ACT_STATIC;
	else
		return ERROR;
	if(OK != tl_get_next_token(pszString, &szString) || NULL == szString)
		return ERROR;
	iDataLen = 0;
	if(TL_TIMER_ACT_COUNTDOWN == pInstr->u.stCommand.state.stTimer.iCurrentAction)
	{			
		iDigit = atoi(szString);
		if(iDigit < 1 || iDigit > 255)
			return ERROR;
		pInstr->u.stCommand.state.stTimer.arrCurrentState[iDataLen++] = iDigit;	
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
			pInstr->u.stCommand.state.stTimer.arrCurrentState[iDataLen++] = iDigit;
			j+=2;
		}
	}
	pInstr->u.stCommand.state.stTimer.iStateLen = iDataLen;
	if(OK != tl_get_next_token(pszString, &szString) || NULL == szString)
		return ERROR;
	if(0 == strncmp(szString, TL_CMD_TIMER_RED, strlen(TL_CMD_TIMER_RED)))
		pInstr->u.stCommand.state.stTimer.iCurrentColor = TL_TIMER_COLOR_RED;
	else if(0 == strncmp(szString, TL_CMD_TIMER_GREEN, strlen(TL_CMD_TIMER_GREEN)))
		pInstr->u.stCommand.state.stTimer.iCurrentColor = TL_TIMER_COLOR_GREEN;
	else
		return ERROR;
	pInstr->iType = TL_INSTR_CMD;
	pInstr->u.stCommand.iDeviceId = pTimer->stHead.iId;
	pInstr->u.stCommand.iType = TL_MODULE_TIMER;
	if( OK !=  tl_timer_command_set(pInstr))
		return ERROR;

	if(TL_IS_DEBUG(CONFIG))
		tl_log_info("Got Timer %d, action 0x%X, color %d, state [0x%X 0x%X 0x%X], len %d", 
			pTimer->stHead.iId, pInstr->u.stCommand.state.stTimer.iCurrentAction, 
			pInstr->u.stCommand.state.stTimer.iCurrentColor, 
			pInstr->u.stCommand.state.stTimer.arrCurrentState[0],
			pInstr->u.stCommand.state.stTimer.arrCurrentState[1],
			pInstr->u.stCommand.state.stTimer.arrCurrentState[2],
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

