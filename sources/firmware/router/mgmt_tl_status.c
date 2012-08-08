#include "os_linux.h"
#include "tl_types.h"
#include "tl_api.h"
#include "tl_log.h"
#include "tl_packets_api.h"
#include "tl_program_api.h"
#include "mgmt_tl_main.h"

void print_help_status()
{
	tl_log_info("\t--status help\tShow this status help");	
	tl_log_info("\t--status system\tShow current system status");
	tl_log_info("\t--status modules\tShow status of all system modules");
	tl_log_info("\t--status program\tShow status of current programs");
}

void get_status()
{
	tl_api_status_t stStatus;
	if(sizeof(stStatus) == tl_mgmt_get(get_client_fd(), (uint8_t*)(&stStatus), sizeof(stStatus)))
	{
		tl_log_info("\tSystem name: %s", stStatus.szCrossName);
		tl_log_info("\t%d devices: %d OK, %d Failed", 
				stStatus.iDeviceNumber, stStatus.iDeviceOk, stStatus.iDeviceError);
		tl_log_info("\tWatchdog interval %d msec", stStatus.iWdInterval);
		tl_log_info("\tCommunication port: %s", stStatus.szComPort);
		tl_log_info("\tCommunication timeout %d msec", stStatus.iCommTimeout);
		tl_log_info("\tProgram in progress: %s", 
				(0==stStatus.szProgInProgress[0]?"None":stStatus.szProgInProgress));
		tl_log_info("\tDebug flags: 0x%X", stStatus.iDebugFlags);
		tl_log_info("\tLog destination: %s", 
			(TL_LOG2NULL==stStatus.iLogDest?"none":
			(TL_LOG2CONSOLE==stStatus.iLogDest?"console":
			(TL_LOG2SYSLOG==stStatus.iLogDest?"syslog":"uknown"))));
	}
}

char *module_type2string(uint8_t iType)
{
	switch(iType)
	{
		case TL_MODULE_MASTER:	return "Master";
		case TL_MODULE_GATE:	return "Gateway";
		case TL_MODULE_LAMP:	return "Lamp";
		case TL_MODULE_TIMER:	return "Timer";
		default: break;
	}
	return "Uknown";
}

char *module_status2string(uint8_t iStatus)
{
	switch(iStatus)
	{
		case TL_MODULE_ERR_OK:			return "OK";
		case TL_MODULE_ERR_INIT:		return "Init";
		case TL_MODULE_ERR_HW:			return "Hardware failure";
		case TL_MODULE_ERR_COMM:		return "Communication error";
		case TL_MODULE_ERR_SLAVE:		return "Slave error detedted";
		case TL_MODULE_ERR_TIMEOUT:		return "Communication timeout";
		case TL_MODULE_ERR_MISMATCH:	return "State mismatch";
		default: break;
	}
	return "Uknown";
}

char *module_color2string(uint8_t iColor)
{
	switch(iColor)
	{
		case TL_TIMER_COLOR_GREEN: return "Green";
		case TL_TIMER_COLOR_RED: return "Red";
		default: break;
	}
	return "Uknown";
}

void printf_status_mod(tl_api_status_mod_t *pModStatus)
{
	tl_log_info("\t %s of type %s at adderess %d, [%s]", 
			pModStatus->szName, module_type2string(pModStatus->iType), 
			pModStatus->iAddress, module_status2string(pModStatus->iStatus));
	if(pModStatus->iGateId)		
		tl_log_info("\t  Connected to gate %d", pModStatus->iGateId);
	
	if(TL_MODULE_LAMP == pModStatus->iType)
	{
		tl_log_info("\t  Segments state: 0x%X", pModStatus->u.stLamp.iState);
	}else if(TL_MODULE_TIMER == pModStatus->iType)
	{
		if(TL_TIMER_ACT_COUNTDOWN == pModStatus->u.stTimer.iCurrentAction)
			tl_log_info("\t  Timer countdown %ds in %s color", 
					pModStatus->u.stTimer.arrCurrentState[0], 
					module_color2string(pModStatus->u.stTimer.iCurrentColor));
		else if(TL_TIMER_ACT_STATIC == pModStatus->u.stTimer.iCurrentAction)
			tl_log_info("\t  Dislay static segments 0x%X 0x%X 0x%X in %s color", 
					pModStatus->u.stTimer.arrCurrentState[0], 
					pModStatus->u.stTimer.arrCurrentState[1], 
					pModStatus->u.stTimer.arrCurrentState[2], 
					module_color2string(pModStatus->u.stTimer.iCurrentColor));
	}
}

void get_status_mod()
{
	uint8_t iModNum, i;
	tl_api_status_mod_t stModStatus;
	
	if(sizeof(iModNum) == tl_mgmt_get(get_client_fd(), (uint8_t*)(&iModNum), sizeof(iModNum) ))
	{
		tl_log_info("\t%d modules in the system:", iModNum);
		for(i=0; i<iModNum; i++)
			if(sizeof(stModStatus) == 
					tl_mgmt_get(get_client_fd(), (uint8_t*)(&stModStatus), sizeof(stModStatus) ))
				printf_status_mod(&stModStatus);
	}
}

void printf_status_instruction(tl_api_status_instr_t *pInstruction)
{

}

void printf_status_program(tl_api_status_prog_t *pProgram)
{

}

void get_status_prog()
{
	uint8_t iProgNum, i, j;
	tl_api_status_prog_t stProgram;
	tl_api_status_instr_t stInstruction;
	
	if(sizeof(iProgNum) == tl_mgmt_get(get_client_fd(), (uint8_t*)(&iProgNum), sizeof(iProgNum) ))
	{
		tl_log_info("\t%d programs are loaded:", iProgNum);
		for(i=0; i<iProgNum; i++)
		{
			if(sizeof(stProgram) == 
					tl_mgmt_get(get_client_fd(), (uint8_t*)(&stProgram), sizeof(stProgram) ))
			{
				printf_status_program(&stProgram);
				for(j=0; i<stProgram.iInstrCount; j++)
					if(sizeof(stInstruction) == 
						tl_mgmt_get(get_client_fd(), (uint8_t*)(&stInstruction), sizeof(stInstruction) ))
						printf_status_instruction(&stInstruction);
			}		
		}
	}	

}

void process_status(char *szArgument, char *szValue)
{
	uint8_t iCommand;

	if(0 == strncmp(szArgument, "help", strlen("help")))
	{
		print_help_status();
		fflush(stdout);
		return;
	}else if(0 == strncmp(szArgument, "system", strlen("help")))	
	{
		iCommand = TL_API_GET_STATUS;
	}else if(0 == strncmp(szArgument, "modules", strlen("modules")))	
	{
		iCommand = TL_API_GET_STATUS_MOD;
	}else if(0 == strncmp(szArgument, "program", strlen("program")))	
	{
		iCommand = TL_API_GET_STATUS_PROG;
	}else
	{
		tl_log_info("Uknown status arguments %s,%s, Usage:", szArgument, szValue);
		print_help_status();
		fflush(stdout);
	}
	
	if(OK == tl_mgmt_send(get_client_fd(), iCommand,  NULL, 0))
	{
		switch(iCommand)
		{
			case TL_API_GET_STATUS:
				get_status();
				return;
			case TL_API_GET_STATUS_MOD:
				get_status_mod();
				return;
			case TL_API_GET_STATUS_PROG:
				get_status_prog();
				return;
			default:
				break;			
		}
	}
}
