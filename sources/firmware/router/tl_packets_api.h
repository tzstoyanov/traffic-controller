#ifndef __TLIGHT_PACKETS_API_H__
#define __TLIGHT_PACKETS_API_H__

#include "tl_common.h"

typedef struct
{
	uint8_t	iCommand;
	uint8_t	iAddress;
	uint8_t	iLen;
	uint8_t	arrData[TL_MAX_PACKET_CAN_DATA];	
}t_can_packet_t;

#endif /* __TLIGHT_PACKETS_API_H__ */

