#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_packets_api.h"
#include "tl_serial_api.h"
#include "tl_program_api.h"
#include "tl_controlers_api.h"
#include "tl_api.h"
#include "lib_tl_log.h"
#include "lib_tl_config.h"
#include "lib_tl_com.h"
#include "lib_tl_packets.h"
#include "lib_tl_client.h"
#include "sim_tl_main.h"

int sim_tl_get_status(tl_api_status_t *pStatus)
{
	int iFd = sim_get_client_fd();
	if(OK == tl_mgmt_send(iFd, TL_API_GET_STATUS, NULL, 0))
		if(sizeof(tl_api_status_t) == tl_mgmt_get(iFd, (uint8_t*)(pStatus), sizeof(tl_api_status_t)))
			return OK;
	return ERROR;	
}

int sim_tl_get_mod(char *szName, tl_api_status_mod_t *pModStatus)
{
	uint8_t iModNum=0;
	int iFd = sim_get_client_fd();
	
	if(OK == tl_mgmt_send(iFd, TL_API_GET_STATUS_MOD, (uint8_t*)szName,strlen(szName) + 1))
		if(	sizeof(iModNum) == tl_mgmt_get(iFd, (uint8_t*)(&iModNum), sizeof(iModNum) &&
			1 == iModNum))
				if(sizeof(tl_api_status_mod_t) == tl_mgmt_get(iFd, (uint8_t*)(pModStatus), sizeof(tl_api_status_mod_t) ))
					return OK;
	return ERROR;	
}



