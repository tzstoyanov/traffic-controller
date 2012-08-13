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
#include "tl_main.h"

typedef int (*tl_api_callback_t)(int iClient, int iFd, uint32_t iCommand, uint32_t iLen, uint8_t *pData);

int tl_api_reply(int iFd, uint8_t *pData, uint32_t iDataLen)
{
	if(-1 == iFd)
		return 0;
	return write(iFd, pData, iDataLen);
}

int tl_api_stop(int iClient, int iFd, uint32_t iCommand, uint32_t iLen, uint8_t *pData)
{
 tl_global_t *pGlobal = tl_get_global();
	
	tl_log_info("Going to exit, stop current program");
	if(pGlobal->pCurrentProgram)
		tl_program_exit(pGlobal->pCurrentProgram, tl_exit_callback, NULL);
	else
		tl_exit_callback(NULL);
	return OK;		
}

int tl_api_set_debug_flag(int iClient, int iFd, uint32_t iCommand, uint32_t iLen, uint8_t *pData)
{
	if(4 != iLen)
		return ERROR;
	tl_debug_switch_set((*((uint32_t*)pData)));
	return OK;
}

int tl_api_set_debug_dest(int iClient, int iFd, uint32_t iCommand, uint32_t iLen, uint8_t *pData)
{
	if(4 != iLen)
		return ERROR;
	tl_log_dest_set(*(uint32_t*)pData);
	return OK;
}

int tl_api_set_program(int iClient, int iFd, uint32_t iCommand, uint32_t iLen, uint8_t *pData)
{
	if(sizeof(tl_api_program_t) != iLen)
		return ERROR;
	return tl_program_change((tl_api_program_t*)pData);
}


int tl_api_get_status(int iClient, int iFd, uint32_t iCommand, uint32_t iLen, uint8_t *pData)
{
	uint32_t iDebugSwitch, iLogDest;
	tl_api_status_t stStatus;
	tl_global_config_t *pGlobalDb = tl_get_global_dbase();
	tl_global_t *pGlobal = tl_get_global();
	
	tl_debug_get(&iDebugSwitch, &iLogDest);
	strncpy(stStatus.szCrossName, pGlobalDb->szCrossName, 128); stStatus.szCrossName[127]=0;
	strncpy(stStatus.szComPort, pGlobalDb->stPort.szPortName, 64); stStatus.szComPort[63]=0;
	stStatus.iWdInterval = pGlobalDb->iWdInterval;
	stStatus.iDeadCount = pGlobalDb->iDeadCount;
	stStatus.iCommTimeout = pGlobalDb->stPort.iTimeout;
	stStatus.iDebugFlags = iDebugSwitch;
	stStatus.iLogDest = iLogDest;
	stStatus.iCritError = pGlobal->iCritError;
	stStatus.iWarning = pGlobal->iWarning;	
	stStatus.szProgLoaded[0]=0;
	if(pGlobal->pCurrentProgram)
	{
		strncpy(stStatus.szProgLoaded, pGlobal->pCurrentProgram->szProgramFile, 128); 
		stStatus.szProgLoaded[127]=0;
		stStatus.iProgState = pGlobal->pCurrentProgram->iState;
		stStatus.iProgLoop = pGlobal->pCurrentProgram->iLoopCount;
	}
	tl_controlers_get_state(&stStatus.iDeviceNumber, &stStatus.iDeviceOk, &stStatus.iDeviceError);
	
	tl_api_reply(iFd, (uint8_t*)&stStatus, sizeof(stStatus));
	return OK;
}

int tl_api_send_mod(int iFd, tl_controler_t *pControler)
{
	tl_api_status_mod_t stMod;

	stMod.iAddress = pControler->iId;
	stMod.iType	= pControler->iType;
	strncpy(stMod.szName, pControler->szName, 64); stMod.szName[63]=0;
	stMod.iGateId = pControler->iGateId;
	stMod.iStatus = pControler->iStatus;
	stMod.iCritical = pControler->iCritical;
	stMod.iErrorCount = pControler->iErrorCount;
	stMod.iWdMiss = pControler->iWdMiss;
	stMod.bLastCommandValid = pControler->bLastCommandValid;
	memcpy(stMod.arrLastCommand, pControler->arrLastCommand, TL_MAX_PACKET_SERIAL_SIZE);
	if(TL_MODULE_LAMP == stMod.iType)
		memcpy(&stMod.u.stLamp, 
				&(((tl_controler_lamp_t*)(pControler))->stCurrentState),	
				sizeof(tl_lamp_state_t ));
	else if	(TL_MODULE_TIMER == stMod.iType)
		memcpy(&stMod.u.stTimer, 
				&(((tl_controler_timer_t*)(pControler))->stCurrentState),	
				sizeof(tl_timer_state_t ));
	
	return tl_api_reply(iFd, (uint8_t*)&stMod, sizeof(stMod));	
}

int tl_api_get_status_mod(int iClient, int iFd, uint32_t iCommand, uint32_t iLen, uint8_t *pData)
{
	uint8_t i, iCount=0;
	tl_controler_t *pControler = NULL;
	tl_global_config_t *pGlobalDb = tl_get_global_dbase();
	
	for(i=1; i<TL_MAX_CONTROLERS; i++)
	{
		if(NULL == pGlobalDb->arrDevices[i])
			break;
		if(pData)
		{	
			if(0 == strncmp(pGlobalDb->arrDevices[i]->szName, (char*)pData, iLen))
			{	
				iCount++;
				pControler = pGlobalDb->arrDevices[i];
				break;
			}
		}else
			iCount++;

	}
	tl_api_reply(iFd, (uint8_t*)&iCount, sizeof(iCount));
	if(pControler)
		return tl_api_send_mod(iFd, pControler);
	if(iCount)
	{
		for(i=1; i<TL_MAX_CONTROLERS; i++)
		{
			if(NULL == pGlobalDb->arrDevices[i])
				break;
			tl_api_send_mod(iFd, pGlobalDb->arrDevices[i]);	
		}
	}
	
	return OK;
}

int tl_api_get_status_prog(int iClient, int iFd, uint32_t iCommand, uint32_t iLen, uint8_t *pData)
{
	uint8_t iCount=0;
	uint32_t i;
	tl_api_status_prog_t stProgram;
	tl_api_status_instr_t stInstr;
	tl_global_t *pGlobal = tl_get_global();
	
	if(NULL == pGlobal->pCurrentProgram)
	{
		tl_api_reply(iFd, (uint8_t*)&iCount, sizeof(iCount));	
		return OK;
	}	
	iCount = 1;
	stProgram.iLoadMode = pGlobal->pCurrentProgram->iLoadMode;
	stProgram.iState = pGlobal->pCurrentProgram->iState;
	stProgram.iLoopCount = pGlobal->pCurrentProgram->iLoopCount;
	stProgram.iLoopStep = pGlobal->pCurrentProgram->iLoopStep;
	stProgram.iExitStep = pGlobal->pCurrentProgram->iExitStep;
	stProgram.iInstrCount = pGlobal->pCurrentProgram->iCount;
	strncpy(stProgram.szFileName, pGlobal->pCurrentProgram->szProgramFile, 512);
	stProgram.szFileName[511]=0;

	tl_api_reply(iFd, (uint8_t*)&iCount, sizeof(iCount));	
	tl_api_reply(iFd, (uint8_t*)&stProgram, sizeof(stProgram));
	
	for(i=0; i<stProgram.iInstrCount; i++)
	{
		memcpy(&(stInstr.stInstruction), 
			&(pGlobal->pCurrentProgram->arrInstructions[i]), sizeof(stInstr.stInstruction) );
		tl_api_reply(iFd, (uint8_t*)&stInstr, sizeof(stInstr));	
	}
	
	return OK;
}

struct
{
	uint32_t iCommand;
	tl_api_callback_t fCallback;
}arrTlApiDispacth[]=
{
	{TL_API_SET_DEBUG_FLAG, tl_api_set_debug_flag},
	{TL_API_SET_DEBUG_DEST, tl_api_set_debug_dest},
	{TL_API_SET_PROGRAM, 	tl_api_set_program},
	{TL_API_GET_STATUS, 	tl_api_get_status},
	{TL_API_GET_STATUS_MOD, tl_api_get_status_mod},
	{TL_API_GET_STATUS_PROG,tl_api_get_status_prog},
	{TL_API_STOP,			tl_api_stop},
};

int tl_api_command_proccess(int iClient, int iFd, uint8_t iCmd, uint8_t iLen, uint8_t *pData)
{
	int i, iTableSize, iRet = ERROR;
	
	iTableSize = sizeof(arrTlApiDispacth)/sizeof(arrTlApiDispacth[0]);	
	for(i=0; i<iTableSize; i++)
		if(iCmd == arrTlApiDispacth[i].iCommand && NULL != arrTlApiDispacth[i].fCallback)
			iRet = arrTlApiDispacth[i].fCallback(iClient, iFd, iCmd, iLen, pData);	

	if(TL_IS_DEBUG(API))
		tl_log_info("Command 0x%X, lenght %d recived from client %d: %s", 
				iCmd, iLen, iClient, OK==iRet?"OK":"Error");

	return OK;
}

int32_t tl_api_client_thread(thread_t *pThread)
{
	int iRet, iFd;
	uint8_t iCommand=0, iLen=0;
	uint8_t *pData = NULL;
	int iClient = (int)pThread->pArg;
	tl_global_t *pGlobal = tl_get_global();
	
	iFd = pThread->iFd;
	if(iClient < TL_API_MAX_CONNECTIONS)
		pGlobal->arrClients[iClient] = NULL;
	
	iRet = read(iFd, &iCommand, 1);
	if(0 >= iRet)
	{
		close(iFd);
		return 0;
	}
	iRet = read(iFd, &iLen, 1);
	if(0 > iRet)
	{
		close(iFd);
		return 0;
	}
	if(iLen > 0)
	{
		pData = malloc(iLen);
		if(NULL == pData)
		{
			close(iFd);
			return 0;
		}	
		iRet = read(iFd, pData, iLen);
		if(0 > iRet)
		{
			close(iFd);
			free(pData);
			return 0;
		}
	}
	
	tl_api_command_proccess(iClient, iFd, iCommand, iLen, pData);
	
	if(iClient < TL_API_MAX_CONNECTIONS)
		pGlobal->arrClients[iClient] = thread_read_add(&(pGlobal->stMaintThread), tl_api_client_thread, (void*)iClient, iFd);
	free(pData);
	return 1;	
}

int32_t tl_api_accept_thread(thread_t *pThread)
{
	int iFd = pThread->iFd, iAccFd, i;
	tl_global_t *pGlobal = (tl_global_t*)pThread->pArg;
	
	iAccFd = accept(pGlobal->iApiFd, NULL, 0);
	
	if(0 <= iAccFd)
	{
		for(i=0; i<TL_API_MAX_CONNECTIONS; i++)
			if(NULL == pGlobal->arrClients[i])
			{
				pGlobal->arrClients[i] = thread_read_add(&(pGlobal->stMaintThread), tl_api_client_thread, (void*)i, iAccFd);
				if(TL_IS_DEBUG(API))
					tl_log_info("New management client %d accepted", i);
				break;	
			}
		if(TL_API_MAX_CONNECTIONS == i)	
		{
			tl_log_error("Cannot accept new management client, limit %d reached", i);
			close(iAccFd);
		}	
	}	
	
	pGlobal->pApiThread = thread_read_add(&(pGlobal->stMaintThread), tl_api_accept_thread, pGlobal, iFd);
	return 1;
}

int tl_api_init()
{
	int iOn=1;
	struct sockaddr_un stSockAddress;
	tl_global_t *pGlobal = tl_get_global();
	
	stSockAddress.sun_family = AF_UNIX;
	sprintf(stSockAddress.sun_path, "%s", TL_API_SOCKET_NAME);
	pGlobal->iApiFd = socket(PF_LOCAL,SOCK_STREAM, 0);
	if(0 > pGlobal->iApiFd)
		return ERROR;
	setsockopt(pGlobal->iApiFd, SOL_SOCKET, SO_REUSEADDR, (void *) &iOn, sizeof(iOn));
	
	unlink(stSockAddress.sun_path);
	
	if( 0 > bind(pGlobal->iApiFd, (struct sockaddr*)&stSockAddress, sizeof(struct sockaddr_un)))
		return ERROR;
	if(0 > listen(pGlobal->iApiFd, TL_API_MAX_CONNECTIONS))
		return ERROR;

	chmod(stSockAddress.sun_path, 
		S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
		
	pGlobal->pApiThread = thread_read_add(&(pGlobal->stMaintThread), tl_api_accept_thread, pGlobal, pGlobal->iApiFd);

	if(TL_IS_DEBUG(INIT))
		tl_log_info("API server %s is ready", TL_API_SOCKET_NAME);
		
	return OK;
}

