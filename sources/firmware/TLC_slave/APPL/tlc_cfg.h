#ifndef TLC_CFG_H
#define TLC_CFG_H

#include "..\..\common\tl_common.h"

/*
 * FRAME :<id:11> <len:4> <data:0-64>
 * <id>  :<cmd:3><address:8>
 */

// address
#define TLC_ADDR_INVALID    0x00
#define TLC_ADDR_ALL        0xFF


// commands
#define TLC_CMD_CONFIG          TL_CMD_CONFIG
#define TLC_CMD_STATE           TL_CMD_STATE
#define TLC_CMD_WD_REQ          TL_CMD_WD_REQUEST
#define TLC_CMD_WD_RESP         TL_CMD_WD_RESPONCE
#define TLC_CMD_WD_RESP_FRAG    TL_CMD_WD_RESPONCE_F

/*
 * config FRAME: <<001><address>> <len:4> [<cfg:4><len:4> <data:0-56>]
 */
#define TLC_CFG_QUERY_INTERVAL      TL_CFGID_QINTERVAL
#define TLC_CFG_DEAD_COUNT          TL_CFGID_DEADCOUNT
#define TLC_CFG_COMM_TIMEOUT        TL_CFGID_COMTIMEOUT
#define TLC_CFG_SUB_MODULES_ADD     TL_CFGID_SLAVES

/*
 * config param QUERY_INTERVAL: <0001> <0001> <query_interval>
 */

/*
 * config param SUB_MODULES: <0010> <size:4> <modules: bytes up to 7>
 */

/*
 * state FRAME: <<010> <address>> <len:4> <type:3> <data:5-61>
 */
#define TLC_TYPE_MASTER     TL_MODULE_MASTER
#define TLC_TYPE_GATEWAY    TL_MODULE_GATE
#define TLC_TYPE_LAMP       TL_MODULE_LAMP
#define TLC_TYPE_TIMER      TL_MODULE_TIMER
#define TLC_TYPE_ALL        TL_MODULE_ALL

/*
 * state FRAME lamp: <<010><address>> <0001> <010><res:1><lamps state:4>
 */

#define TLC_LAMP_OFF        TL_LAMP_STATE_OFF
#define TLC_LAMP_ON         TL_LAMP_STATE_ON

/*
 * state FRAME timer: <<010><address>> <len:4> <011><color:2><action:3><action data>
 */
#define TLC_TIMER_COLOR_GREEN     TL_TIMER_COLOR_GREEN
#define TLC_TIMER_COLOR_RED       TL_TIMER_COLOR_RED

#define TLC_TIMER_ACTION_COUNT      TL_TIMER_ACT_COUNTDOWN      // 1 byte count reload
#define TLC_TIMER_ACTION_STATIC     TL_TIMER_ACT_STATIC         // 1 byte segment data

/*
 * frame WD_REQ <<011> <address>> <0001> <resp address:8>
 */

/*
 * frame WD_RESP lamp  <<100> <address>> <0011> <slave address:8> <010> <status:1><error:4> <:4><lamps state:4>
 * frame WD_RESP timer <<100> <address>> <0100> <slave address:8> <011> <status:1><error:4> <:3><color:2><action:3><action data:8>
 * frame WD_RESP GW    <<100> <address>> <len:> [<slave address:8> <type:3> <status:1><error/state:4>]
 * frame WD_RESP GW    <<101> <address>> <len:> [<slave address:8> <type:3> <status:1><error/state:4>]
 */

#define TLC_STATUS_OK       TL_MODULE_STATUS_OK
#define TLC_STATUS_FAIL     TL_MODULE_STATUS_FAIL

#define TLC_ERROR_OK        TL_MODULE_STATE_OK
#define TLC_ERROR_INIT      TL_MODULE_STATE_INIT
#define TLC_ERROR_OUTPUT    TL_MODULE_STATE_HW_ERR
#define TLC_ERROR_WARNING   TL_MODULE_STATE_HW_WARN
#define TLC_ERROR_COMM      TL_MODULE_STATE_COMM_ERR
#define TLC_ERROR_TIMEOUT   TL_MODULE_STATE_TIMEOUT
#define TLC_ERROR_MISMATCH  TL_MODULE_STATE_MISMATCH

#endif
