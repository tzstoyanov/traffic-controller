#ifndef __TLIGHT_SERIAL_H__
#define __TLIGHT_SERIAL_H__

#include "tl_serial_api.h"

int tl_com_port_open(tl_com_t *pCom);
int32_t tl_com_accept(thread_t *pThread);
int tl_com_send_session(tl_com_t *pCom, uint8_t iAddress, uint8_t iNextState);
int32_t tl_com_read(thread_t *pThread);

#endif /* __TLIGHT_SERIAL_H__ */

