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

typedef int (*sim_utest_hook_t)(tl_controler_t *pControler, t_can_packet_t *pPacket);

int sim_utest_normal(tl_controler_t *pControler, t_can_packet_t *pPacket)
{
	
	return OK;
}

struct
{
	sim_utest_hook_t fHook;
	char *szTestName;
}arrSimUtest[]={
	{sim_utest_normal, "Normal operation"}
};

int sim_tl_utest_init()
{	
//	int i;
	tl_sim_global_t *pGlobal = tl_get_global_sim();
//	tl_global_config_t *pDbGlobal = tl_get_global_dbase();
	
	if(0 == pGlobal->iUtest)
		return OK;
	tl_log_info("Start unit test");
	
	return OK;
}

