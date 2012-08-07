#ifndef __TLIGHT_MGMT_CLIENT_H__
#define __TLIGHT_MGMT_CLIENT_H__

int tl_mgmt_connect();
int tl_mgmt_send(int iFd, uint8_t iCommand, uint8_t iLen, uint8_t *pData);

#endif /* __TLIGHT_MGMT_CLIENT_H__ */


