#ifndef __TLIGHT_API_H__
#define __TLIGHT_API_H__

#define TL_API_MAX_CONNECTIONS	5
#define TL_API_SOCKET_NAME	"/tmp/tlight_api_sock"

#define TL_API_SET_DEBUG_FLAG	1
#define TL_API_SET_DEBUG_DEST	2
#define TL_API_SET_DEBUG_GET	3

int tl_api_init();

#endif /* __TLIGHT_API_H__ */


