#ifndef __TLIGHT_DBASE_H__
#define __TLIGHT_DBASE_H__

#include "tl_controlers_api.h"

tl_controler_t *tl_NewControler(uint8_t iType);
tl_controler_t *tl_get_controler_byname(char *szName);
int tl_check_controlers();
int tl_ask_controler_state(uint8_t iAddress);
int tl_update_controler_state(tl_instruction_t *pCommand);
int tl_controler_timeout(uint8_t iAddress);
int tl_controlers_get_state(uint8_t *pAll, uint8_t *pOk, uint8_t *pFail);
int tl_send_failsafe();
int tl_send_controlers_config(uint8_t iAddress);

#endif /* __TLIGHT_DBASE_H__ */
