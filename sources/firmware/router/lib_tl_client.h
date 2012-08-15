#ifndef __TLIGHT_LIB_CLIENT_H__
#define __TLIGHT_LIB_CLIENT_H__

int tl_mgmt_connect();
int tl_mgmt_send(int iFd, uint8_t iCommand, uint8_t *pData, uint32_t iLen);
int tl_mgmt_get(int iFd, uint8_t *pData, uint32_t iLen);

#endif /* __TLIGHT_LIB_CLIENT_H__ */
