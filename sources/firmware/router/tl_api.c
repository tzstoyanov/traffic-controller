#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_api.h"
#include "tl_serial.h"
#include "tl_program.h"
#include "tl_packets.h"
#include "tl_controlers.h"
#include "tl_config.h"
#include "tl_log.h"

typedef int (*tl_api_callback_t)(int iClient, uint32_t iCommand, uint32_t iLen, uint8_t *pData);

int tl_api_set_debug_flag(int iClient, uint32_t iCommand, uint32_t iLen, uint8_t *pData)
{
	if(4 != iLen)
		return ERROR;
	tl_debug_switch_set((*((uint32_t*)pData)));
	return OK;
}

int tl_api_set_debug_dest(int iClient, uint32_t iCommand, uint32_t iLen, uint8_t *pData)
{
	if(4 != iLen)
		return ERROR;
	tl_log_dest_set(*(uint32_t*)pData);
	return OK;
}

struct
{
	uint32_t iCommand;
	tl_api_callback_t fCallback;
}arrTlApiDispacth[]=
{
	{TL_API_SET_DEBUG_FLAG, tl_api_set_debug_flag},
	{TL_API_SET_DEBUG_DEST, tl_api_set_debug_dest}
};

int tl_api_command_proccess(int iClient, uint8_t iCmd, uint8_t iLen, uint8_t *pData)
{
	int i, iTableSize, iRet = ERROR;
	
	iTableSize = sizeof(arrTlApiDispacth)/sizeof(arrTlApiDispacth[0]);	
	for(i=0; i<iTableSize; i++)
		if(iCmd == arrTlApiDispacth[i].iCommand && NULL != arrTlApiDispacth[i].fCallback)
			iRet = arrTlApiDispacth[i].fCallback(iClient, iCmd, iLen, pData);	

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
	
	tl_api_command_proccess(iClient, iCommand, iLen, pData);
	
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

