#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_api.h"
#include "lib_tl_log.h"
#include "tl_packets_api.h"
#include "tl_program_api.h"
#include "mgmt_tl_main.h"

void print_help_status()
{
	tl_log_info("\t--status help\t\tShow this status help");	
	tl_log_info("\t--status system\t\tShow current system status");
	tl_log_info("\t--status module\t\tShow status of all system modules");
	tl_log_info("\t--status module=NAME\tShow status of requested system module");
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
		tl_log_info("\tCritical errors %d, warnings %d", stStatus.iCritError ,stStatus.iWarning );		
		tl_log_info("\tWatchdog interval %d msec", stStatus.iWdInterval);
		tl_log_info("\tDead Count %d", stStatus.iDeadCount);
		tl_log_info("\tCommunication port: %s", stStatus.szComPort);
		tl_log_info("\tCommunication timeout %d msec", stStatus.iCommTimeout);
		tl_log_info("\tLoaded program: %s", 
				(0==stStatus.szProgLoaded[0]?"None":stStatus.szProgLoaded));
		if(0 != stStatus.szProgLoaded[0])
			tl_log_info("\tProgram is in state %s, executed %d times",
			 		tl_programstate2string(stStatus.iProgState), stStatus.iProgLoop);
		tl_log_info("\tDebug flags: 0x%X", stStatus.iDebugFlags);
		tl_log_info("\tLog destination: %s", 
			(TL_LOG2NULL==stStatus.iLogDest?"none":
			(TL_LOG2CONSOLE==stStatus.iLogDest?"console":
			(TL_LOG2SYSLOG==stStatus.iLogDest?"syslog":"uknown"))));
	}
}

void print_status_lamp(char *szGaps, tl_lamp_state_t *pLamp)
{
	tl_log_info("%sSegments state: 0x%X", szGaps, pLamp->iState);
}

void print_status_timer(char *szGaps, tl_timer_state_t *pTimer)
{
	if(TL_TIMER_ACT_COUNTDOWN == pTimer->iAction)
		tl_log_info("%sTimer countdown %ds in %s color", szGaps,
				pTimer->arrState[0], 
				tl_color2string(pTimer->iColor));
	if(TL_TIMER_ACT_STATIC == pTimer->iAction)
		tl_log_info("%sDisplay static segments 0x%X 0x%X 0x%X in %s color", szGaps,
					pTimer->arrState[0], pTimer->arrState[1], 
					pTimer->arrState[2], tl_color2string(pTimer->iColor));
}

void print_status_mod(tl_api_status_mod_t *pModStatus)
{
	tl_log_info("\t %s of type %s at adderess %d, [%s]", 
			pModStatus->szName, tl_type2string(pModStatus->iType), 
			pModStatus->iAddress, tl_status2string(pModStatus->iStatus));
	if(pModStatus->iGateId)		
		tl_log_info("\t  Connected to gate %d, Errors %d, WdMiss %d, %s", 
				pModStatus->iGateId, pModStatus->iErrorCount, pModStatus->iWdMiss,
				pModStatus->iCritical?"Critical":"Not critical" );
	else
		tl_log_info("\t  Errors %d, WdMiss %d, %s", 
				pModStatus->iErrorCount, pModStatus->iWdMiss,
				pModStatus->iCritical?"Critical":"Not critical");		
	if(pModStatus->bLastCommandValid)
		tl_log_info("\t  Last valid command [%s]", pModStatus->arrLastCommand);	
	if(TL_MODULE_LAMP == pModStatus->iType)
		print_status_lamp("\t  ", &(pModStatus->u.stLamp));
	if(TL_MODULE_TIMER == pModStatus->iType)
		print_status_timer("\t  ", &(pModStatus->u.stTimer));
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
				print_status_mod(&stModStatus);
	}
}

void print_cmd_instruction(tl_api_status_instr_t *pInstruction)
{
	tl_log_info("\t   [%s]: %s at address %d, ", 
		pInstruction->stInstruction.u.stCommand.arrCommand,
		tl_type2string(pInstruction->stInstruction.u.stCommand.iType),
		pInstruction->stInstruction.u.stCommand.iDeviceId);
	if(TL_MODULE_LAMP == pInstruction->stInstruction.u.stCommand.iType)
		print_status_lamp("\t    ", &(pInstruction->stInstruction.u.stCommand.state.stLamp));
	if(TL_MODULE_TIMER == pInstruction->stInstruction.u.stCommand.iType)
		print_status_timer("\t    ", &(pInstruction->stInstruction.u.stCommand.state.stTimer));
}

void print_status_program(tl_api_status_prog_t *pProgram)
{
	tl_log_info("\t Program %s, %s:", 
		pProgram->szFileName, tl_programstate2string(pProgram->iState));
	tl_log_info("\t  Executed %d times, loop step %d, exit step %d, %d instructions:", 
			pProgram->iLoopCount, pProgram->iLoopStep, 
			pProgram->iExitStep, pProgram->iInstrCount);
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
				print_status_program(&stProgram);
				tl_log_info("   INIT stage:");
				for(j=0; j<stProgram.iInstrCount; j++)
				{
					if(sizeof(stInstruction) == 
						tl_mgmt_get(get_client_fd(), (uint8_t*)(&stInstruction), sizeof(stInstruction) ))
					{
						if(j == stProgram.iLoopStep)
							tl_log_info("   LOOP stage:");
						if(j == stProgram.iExitStep)
							tl_log_info("   EXIT stage:");							
						switch(stInstruction.stInstruction.iType)
						{	
							case TL_INSTR_DELAY: 
								tl_log_info("\t   Delay %d ms", stInstruction.stInstruction.u.iDelay);
								break;
							case TL_INSTR_CMD:
								print_cmd_instruction(&stInstruction);
								break;
							default:
								tl_log_info("\t   Unkown %d", stInstruction.stInstruction.iType);
								break;
						}	
					}		
				}	
			}
		}	

	}

}		
void process_status(char *szArgument, char *szValue)
{
	uint8_t *pArg = NULL;
	uint32_t iArgLen = 0;
	uint8_t iCommand;
	
	if(0 == strncmp(szArgument, "help", strlen("help")))
	{
		print_help_status();
		fflush(stdout);
		return;
	}else if(0 == strncmp(szArgument, "system", strlen("help")))	
	{
		iCommand = TL_API_GET_STATUS;
	}else if(0 == strncmp(szArgument, "module", strlen("modules")))	
	{
		iCommand = TL_API_GET_STATUS_MOD;
		if(szValue)
		{
			pArg = (uint8_t*)szValue;
			iArgLen = strlen(szValue) + 1;		
		}
	}else if(0 == strncmp(szArgument, "program", strlen("program")))	
	{
		iCommand = TL_API_GET_STATUS_PROG;
	}else
	{
		tl_log_info("Uknown status arguments %s,%s, Usage:", szArgument, szValue);
		print_help_status();
		fflush(stdout);
		return;
	}
	
	if(OK == tl_mgmt_send(get_client_fd(), iCommand, pArg, iArgLen))
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

