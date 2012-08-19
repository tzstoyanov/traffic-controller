#ifndef NVRAM_CFG_H
#define NVRAM_CFG_H

#include "system.h"

#if DEVICE_TYPE == DEVICE_TYPE_LAMP
    #define nvram_get_u8DeviceId()       0x01
#elif DEVICE_TYPE == DEVICE_TYPE_TIMER
    #define nvram_get_u8DeviceId()       0x02
#else
    #define nvram_get_u8DeviceId()       0x42
#endif


#endif
