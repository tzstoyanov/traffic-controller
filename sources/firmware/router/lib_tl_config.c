#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "lib_tl_log.h"
#include "tl_serial_api.h"
#include "tl_packets_api.h"
#include "tl_controlers_api.h"
#include "lib_tl_config.h"

static tl_global_config_t stGlobalConfigDb;

tl_global_config_t *tl_get_global_dbase()
{
	return &stGlobalConfigDb;
}
int tl_get_next_token(char **pszLine, char **pszString)
{
	int i=0, j=0;
	while(0 != (*pszLine)[i] && !isalnum((*pszLine)[i]) && '/' != (*pszLine)[i]) i++;
	if(0 == (*pszLine)[i])
		return ERROR;
	j=i;
	while(	isalnum((*pszLine)[j]) || '.' == ((*pszLine)[j]) 
			|| '/' == ((*pszLine)[j]) || '_' == ((*pszLine)[j]))  j++;
	if(0 == (*pszLine)[j])
		return ERROR;
	(*pszString) = ((*pszLine) + i);
	(*pszLine)[j] = 0;
	(*pszLine) = ((*pszLine) + j + 1);
	return OK;
}

int tl_parse_port_cfg(char *szPortCfg, tl_com_t *pCom)
{
	int i=0, iLen = strlen(szPortCfg);

	pCom->iPortFlags = 0;
	
	while(szPortCfg[i] && '/' != szPortCfg[i]) i++;
	if('/' != szPortCfg[i] || (i+3) >= iLen)
		return ERROR;
	szPortCfg[i] = 0;
	
	if(0 == strncmp(szPortCfg, "9600", strlen("9600")))
		pCom->iPortSpeed	= B9600;
	else 	if(0 == strncmp(szPortCfg, "19200", strlen("19200")))
			pCom->iPortSpeed	= B19200;
	else 	if(0 == strncmp(szPortCfg, "38400", strlen("38400")))
			pCom->iPortSpeed	= B38400;
	else 	if(0 == strncmp(szPortCfg, "57600", strlen("57600")))
			pCom->iPortSpeed	= B57600;
	else 	if(0 == strncmp(szPortCfg, "115200", strlen("115200")))
			pCom->iPortSpeed	= B115200;
	else 	if(0 == strncmp(szPortCfg, "230400", strlen("230400")))
			pCom->iPortSpeed	= B230400;
	else 	if(0 == strncmp(szPortCfg, "460800", strlen("460800")))
			pCom->iPortSpeed	= B460800;
	else 	if(0 == strncmp(szPortCfg, "500000", strlen("500000")))
			pCom->iPortSpeed	= B500000;
	else 	if(0 == strncmp(szPortCfg, "576000", strlen("576000")))
			pCom->iPortSpeed	= B576000;
	else 	if(0 == strncmp(szPortCfg, "921600", strlen("921600")))
			pCom->iPortSpeed	= B921600;
	else 	if(0 == strncmp(szPortCfg, "1000000", strlen("1000000")))
			pCom->iPortSpeed	= B1000000;
	else 	if(0 == strncmp(szPortCfg, "1152000", strlen("1152000")))
			pCom->iPortSpeed	= B1152000;
	else 	if(0 == strncmp(szPortCfg, "1500000", strlen("1500000")))
			pCom->iPortSpeed	= B1500000;
	else return ERROR;

	switch(szPortCfg[i+1])
	{
		case '5':
				pCom->iPortFlags |= CS5;
			break;
		case '6':
				pCom->iPortFlags |= CS6;
			break;
		case '7':
				pCom->iPortFlags |= CS7;
			break;
		case '8':
				pCom->iPortFlags |= CS8;
			break;
		default:
			return ERROR;
	}	

	switch(szPortCfg[i+2])
	{
		case 'N':
			break;
		case 'E':
				pCom->iPortFlags |= PARENB;
			break; 
		case 'O':
				pCom->iPortFlags |= PARENB;
				pCom->iPortFlags |= PARODD;
			break;
		default:
			return ERROR;
	}
	
	if(TL_IS_DEBUG(CONFIG))
		tl_log_info("Got port speed %s (0x%X), data %c , parity %c (0x%X)", 
					szPortCfg, szPortCfg[i+1], szPortCfg[i+2], pCom->iPortFlags) ;
	
	return OK;	
}

int tl_parse_cfg_file(char *szFileName, tl_global_config_t *pGlobal, tl_new_controler_callback_t fNewControler)
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
			while(iLen 	&& !isalnum(szToken[iLen]) 
						&& (' ' != szToken[iLen])
						&& ('\t' != szToken[iLen]) )			
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
			if(0 == strncmp(szString, TL_COM_SERIAL_PREFIX, sizeof(TL_COM_SERIAL_PREFIX)))
				pGlobal->stPort.iType = TL_COM_SERIAL;
			else	
				pGlobal->stPort.iType = TL_COM_LOCALSOCK;
			if(TL_IS_DEBUG(CONFIG))
				tl_log_info("Got port name [%s], type %s", 
					pGlobal->stPort.szPortName, 
					TL_COM_SERIAL==pGlobal->stPort.iType?"Serial":"Simulation");
		}else if(0 == strncmp(szString, TL_PARAM_COM_CFG, strlen(TL_PARAM_COM_CFG)))
		{
			szString = NULL;
			if(OK != tl_get_next_token(&szToken, &szString) || NULL == szString)
				continue;
			if(TL_IS_DEBUG(CONFIG))
				tl_log_info("Got port params [%s], going to parse it", szString);							
			if(OK != tl_parse_port_cfg(szString, &(pGlobal->stPort)))
				continue;
		}else if(0 == strncmp(szString, TL_PARAM_WD, strlen(TL_PARAM_WD)))
		{
			szString = NULL;
			if(OK != tl_get_next_token(&szToken, &szString) || NULL == szString)
				continue;
			pGlobal->iWdInterval = atoi(szString);	
			if(TL_IS_DEBUG(CONFIG)) 
				tl_log_info("Got WD interval [%d]", pGlobal->iWdInterval);
		}else if(0 == strncmp(szString, TL_PARAM_DEADCOUNT, strlen(TL_PARAM_TIMEOUT)))
		{
			szString = NULL;
			if(OK != tl_get_next_token(&szToken, &szString) || NULL == szString)
				continue;
			pGlobal->iDeadCount = atoi(szString);	
			if(TL_IS_DEBUG(CONFIG)) 
				tl_log_info("Got Dead Count [%d]", pGlobal->iDeadCount);						
		}else if(0 == strncmp(szString, TL_PARAM_TIMEOUT, strlen(TL_PARAM_TIMEOUT)))
		{
			szString = NULL;
			if(OK != tl_get_next_token(&szToken, &szString) || NULL == szString)
				continue;
			pGlobal->stPort.iTimeout = atoi(szString);
			if(TL_IS_DEBUG(CONFIG))
				tl_log_info("Got Timeout interval [%d]", pGlobal->stPort.iTimeout);			
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
			pControler = fNewControler(iType);
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


