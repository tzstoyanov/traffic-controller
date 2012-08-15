#include "os_linux.h"
#include "linux_thread.h"
#include "tl_types.h"
#include "tl_api.h"
#include "lib_tl_log.h"
#include "lib_tl_client.h"
#include "mgmt_tl_main.h"

void print_help_program()
{
	tl_log_info("\t--program help\tShow this status help");	
	tl_log_info("\t--program replace=FILENAME\tReplace current program with this one. Specify full path to the new program");
}

void process_program(char *szArgument, char *szValue)
{
	tl_api_program_t stProgram;

	if(0 == strncmp(szArgument, "help", strlen("help")))
	{
		print_help_program();
		fflush(stdout);
		return;
	}else if(0 == strncmp(szArgument, "replace", strlen("replace")))	
	{
		stProgram.iType = TL_PROGRAM_REPLACE;
		strncpy(stProgram.szProgName, szValue, 128); stProgram.szProgName[127]=0;
	}else
	{
		tl_log_info("Uknown status arguments %s,%s, Usage:", szArgument, szValue);
		print_help_status();
		fflush(stdout);
		return;
	}
	
	tl_mgmt_send(get_client_fd(), TL_API_SET_PROGRAM, (uint8_t*)&stProgram, sizeof(stProgram) );
}

